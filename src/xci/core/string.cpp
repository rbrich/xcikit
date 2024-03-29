// string.cpp created on 2018-03-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "string.h"

#include "parser/unescape.h"

#include <xci/core/log.h>
#include <xci/compat/macros.h>

#include <fmt/core.h>
#include <widechar_width/widechar_width.h>

#include <cctype>
#include <locale>
#include <codecvt>
#include <cassert>

namespace xci::core {

using std::string;
using std::vector;
using std::string_view;
using fmt::format;


bool remove_prefix(string& str, const string& prefix)
{
    if (!str.starts_with(prefix))
        return false;
    str.erase(0, prefix.size());
    return true;
}


bool remove_suffix(string& str, const string& suffix)
{
    if (!str.ends_with(suffix))
        return false;
    str.erase(str.size() - suffix.size());
    return true;
}


std::string replace_all(std::string_view str, std::string_view substring, std::string_view replacement)
{
    std::string output;
    size_t pos = 0;
    auto end = str.cbegin();
    for(;;) {
        pos = str.find(substring, pos);
        if (pos == std::string::npos)
            break;
        output.append(end, str.cbegin() + pos);
        output += replacement;
        pos += substring.size();
        end = str.cbegin() + pos;
    }
    output.append(end, str.cend());
    return output;
}


template <class TDelim>
vector<string_view> _split(string_view str, TDelim delim, size_t delim_len, int maxsplit)
{
    std::vector<string_view> res;
    size_t pos = 0;
    while (maxsplit != 0) {
        size_t end = str.find(delim, pos);
        if (end == string_view::npos)
            break;
        res.push_back(str.substr(pos, end - pos));
        pos = end + delim_len;
        --maxsplit;
    }
    res.push_back(str.substr(pos, str.size() - pos));
    return res;
}

std::vector<std::string_view> split(std::string_view str, char delim, int maxsplit) { return _split(str, delim, 1, maxsplit); }
std::vector<std::string_view> split(std::string_view str, std::string_view delim, int maxsplit)  { return _split(str, delim, delim.size(), maxsplit); }

vector<string_view> split_ws(string_view str, int maxsplit)
{
    constexpr const char* whitespace = " \n\r\t\v\f";
    std::vector<string_view> res;
    size_t pos = str.find_first_not_of(whitespace);
    while (pos != string_view::npos && maxsplit != 0) {
        size_t end = str.find_first_of(whitespace, pos);
        if (end == string_view::npos)
            break;
        res.push_back(str.substr(pos, end - pos));
        pos = str.find_first_not_of(whitespace, end);
        --maxsplit;
    }
    if (pos != string_view::npos)
        res.push_back(str.substr(pos, str.size() - pos));
    return res;
}

template <class TDelim>
vector<string_view> _rsplit(string_view str, TDelim delim, size_t delim_len, int maxsplit)
{
    std::vector<string_view> res;
    size_t pos = str.size();
    while (maxsplit != 0 && pos != 0) {
        size_t beg = str.rfind(delim, pos - 1);
        if (beg == string_view::npos)
            break;
        res.insert(res.begin(), str.substr(beg + delim_len, pos - beg - delim_len));
        pos = beg;
        --maxsplit;
    }
    res.insert(res.begin(), str.substr(0, pos));
    return res;
}

std::vector<std::string_view> rsplit(std::string_view str, char delim, int maxsplit)  { return _rsplit(str, delim, 1, maxsplit); }
std::vector<std::string_view> rsplit(std::string_view str, std::string_view delim, int maxsplit)  { return _rsplit(str, delim, delim.size(), maxsplit); }


std::string escape(string_view str, bool extended, bool utf8)
{
    std::string out;
    out.reserve(str.size());
    for (auto cp = str.begin(); cp != str.end(); ++cp) {
        switch (*cp) {
            case '\a': out += "\\a"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\v': out += "\\v"; break;
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\'': out += "\\'"; break;
            default: {
                if (extended && *cp == '\x1b') {
                    out += "\\e";
                    break;
                }
                auto c_int = (int)(unsigned char)(*cp);
                if (std::isprint(c_int))
                    out += *cp;
                else {
                    if (utf8) {
                        auto len = utf8_char_length(*cp);
                        if (len > 1 && cp + len <= str.end()) {
                            // multi-byte UTF-8 char -> passthrough
                            out.append(cp, cp + size_t(len));
                            cp += len - 1;
                            break;
                        }
                    }
                    out += format("\\x{:02x}", c_int);
                }
                break;
            }
        }
    }
    return out;
}


// override control to avoid demangled rules appearing in binary static data
// (even though they're never used, they don't get optimized away)
template< typename Rule >
struct UnescapeControl : tao::pegtl::normal< Rule >
{
    template< typename Input, typename... States >
    static void raise( const Input& in, States&&...) {
        throw tao::pegtl::parse_error( "parse error", in );
    }
};

template<typename Rule>
std::string gen_unescape(string_view str)
{
    tao::pegtl::memory_input<
        tao::pegtl::tracking_mode::eager,
        tao::pegtl::eol::lf_crlf,
        const char*>
    input(str, "<input>");
    std::string result;
    result.reserve(str.size());

    try {
        auto matched = tao::pegtl::parse< Rule, parser::unescape::Action, UnescapeControl >( input, result );
        assert(matched);
        (void) matched;
    } catch (tao::pegtl::parse_error&) {
        assert(!"unescape: parse error");
    }
    result.shrink_to_fit();
    return result;
}

std::string unescape(string_view str) { return gen_unescape<parser::unescape::String>(str); }
std::string unescape_uni(string_view str) { return gen_unescape<parser::unescape::StringUni>(str); }


std::string to_lower(std::string_view str)
{
    std::string result(str.size(), '\0');
    std::transform(
        str.begin(), str.end(),
        result.begin(), [](char c){ return (char) std::tolower(c); });
    return result;
}


bool ci_equal(std::string_view s1, std::string_view s2)
{
    return std::equal(s1.begin(), s1.end(), s2.begin(), s2.end(),
                      [](char a, char b) { return std::tolower(a) == std::tolower(b); });
}


std::u32string to_utf32(string_view utf8)
{
    XCI_IGNORE_DEPRECATED(
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert_utf32;
    )
    try {
        return convert_utf32.from_bytes(utf8.data(), utf8.data() + utf8.size());
    } catch (const std::range_error& e) {
        log::error("to_utf32: Invalid UTF8 string: {} ({})", utf8, e.what());
        return {};
    }
}


template <class Elem>
std::string _to_utf8(std::basic_string_view<Elem> wstr)
{
    XCI_IGNORE_DEPRECATED(
    std::wstring_convert<std::codecvt_utf8<Elem>, Elem> convert;
    )
    try {
        return convert.to_bytes(wstr.data(), wstr.data() + wstr.size());
    } catch (const std::range_error& e) {
        log::error("to_utf8: Invalid UTF16/32 string ({})", e.what());
        return {};
    }
}

std::string to_utf8(std::u16string_view wstr) { return _to_utf8(wstr); }
std::string to_utf8(std::u32string_view wstr) { return _to_utf8(wstr); }

#ifdef _WIN32
std::string to_utf8(std::wstring_view wstr) { return _to_utf8(wstr); }
#endif

std::string to_utf8(char32_t codepoint)
{
    XCI_IGNORE_DEPRECATED(
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert_utf32;
    )
    return convert_utf32.to_bytes(codepoint);
}


int utf8_char_length(char first)
{
    if (first == 0) {
        return 0;
    }
    if ((first & 0b10000000) == 0) {
        // 0xxxxxxx -> 1 byte
        return 1;
    }
    if ((first & 0b11100000) == 0b11000000) {
        // 110xxxxx -> 2 bytes
        return 2;
    }
    if ((first & 0b11110000) == 0b11100000) {
        // 1110xxxx -> 3 bytes
        return 3;
    }
    if ((first & 0b11111000) == 0b11110000) {
        // 11110xxx -> 4 bytes
        return 4;
    }
    log::error("utf8_char_length: Invalid UTF8 string, encountered code 0x{:02x}", int(first));
    return 1;
}


const char* utf8_prev(const char* utf8)
{
    while ((static_cast<unsigned char>(*utf8) & 0b11000000) == 0b10000000)
        --utf8;
    return utf8 - 1;
}


size_t utf8_length(std::string_view str)
{
    size_t length = 0;
    for (auto pos = str.cbegin(); pos != str.cend(); pos = utf8_next(pos)) {
        ++length;
    }
    return length;
}


size_t utf8_offset(std::string_view str, size_t n_chars)
{
    return utf8_offset_iter(str.cbegin(), str.cend(), n_chars) - str.cbegin();
}


string_view utf8_substr(string_view str, size_t pos, size_t count)
{
    auto begin = utf8_offset_iter(str.cbegin(), str.cend(), pos);
    auto end = utf8_offset_iter(begin, str.cend(), count);
    return {&*begin, size_t(end - begin)};
}


char32_t utf8_codepoint(const char* utf8)
{
    char c0 = utf8[0];
    if ((c0 & 0x80) == 0) {
        // 0xxxxxxx -> 1 byte
        return char32_t(c0 & 0x7f);
    }
    if ((c0 & 0xe0) == 0xc0) {
        // 110xxxxx -> 2 bytes
        return char32_t(((c0 & 0x1f) << 6) | (utf8[1] & 0x3f));
    }
    if ((c0 & 0xf0) == 0xe0) {
        // 1110xxxx -> 3 bytes
        return char32_t(((c0 & 0x0f) << 12) | ((utf8[1] & 0x3f) << 6) | (utf8[2] & 0x3f));
    }
    if ((c0 & 0xf8) == 0xf0) {
        // 11110xxx -> 4 bytes
        return char32_t(((c0 & 0x07) << 18) | ((utf8[1] & 0x3f) << 12) | ((utf8[2] & 0x3f) << 6) | (utf8[3] & 0x3f));
    }
    log::error("utf8_codepoint: Invalid UTF8 string, encountered code {:02x}", int(c0));
    return 0;
}


std::pair<int, char32_t> utf8_codepoint_and_length(std::string_view utf8)
{
    if (utf8.empty())
        return {0, 0};
    char c0 = utf8[0];
    if ((c0 & 0x80) == 0) {
        // 0xxxxxxx -> 1 byte
        return {1, char32_t(c0 & 0x7f)};
    }
    if (utf8.size() == 1)
        return {0, 0};
    if ((c0 & 0xe0) == 0xc0) {
        // 110xxxxx -> 2 bytes
        return {2, char32_t(((c0 & 0x1f) << 6) | (utf8[1] & 0x3f))};
    }
    if (utf8.size() == 2)
        return {0, 0};
    if ((c0 & 0xf0) == 0xe0) {
        // 1110xxxx -> 3 bytes
        return {3, char32_t(((c0 & 0x0f) << 12) | ((utf8[1] & 0x3f) << 6) | (utf8[2] & 0x3f))};
    }
    if (utf8.size() == 3)
        return {0, 0};
    if ((c0 & 0xf8) == 0xf0) {
        // 11110xxx -> 4 bytes
        return {4, char32_t(((c0 & 0x07) << 18) | ((utf8[1] & 0x3f) << 12) | ((utf8[2] & 0x3f) << 6) | (utf8[3] & 0x3f))};
    }
    return {0, -1};
}


size_t utf8_partial_end(string_view str)
{
    // Single byte from multi-byte UTF-8 char?
    if (str.length() < 1)
        return 0;
    // Last byte must initiate 2, 3 or 4-byte sequence.
    char back0 = str.back();
    if ((back0 & 0xe0) == 0xc0 || (back0 & 0xf0) == 0xe0 || (back0 & 0xf8) == 0xf0)
        return 1;

    // Two bytes from multi-byte UTF-8 char? Last byte must be continuation byte.
    if (str.length() < 2 || (back0 & 0xc0) != 0x80)
        return 0;
    // Second last byte must initiate 3 or 4-byte sequence.
    char back1 = str[str.size() - 2];
    if ((back1 & 0xf0) == 0xe0 || (back1 & 0xf8) == 0xf0)
        return 2;

    // Three bytes from 4-byte UTF-8 char? Second last byte must be continuation byte.
    if (str.length() < 3 || (back1 & 0xc0) != 0x80)
        return 0;
    // Third last byte must initiate 4-byte sequence.
    char back2 = str[str.size() - 3];
    if ((back2 & 0xf8) == 0xf0)
        return 3;

    // The UTF-8 sequence is properly closed.
    return 0;
}


unsigned c32_width(char32_t c)
{
    using namespace wcw;
    int w = widechar_wcwidth(c);
    switch (w) {
        case widechar_combining:
            return 0;

        case widechar_nonprint:
        case widechar_ambiguous:
        case widechar_private_use:
        case widechar_unassigned:
            return 1;

        case widechar_widened_in_9:
            return 2;

        default:
            return w < 0 ? 1 : unsigned(w);
    }
}


size_t utf8_width(std::string_view str)
{
    size_t w = 0;
    for (auto pos = str.cbegin(); pos != str.cend(); pos = utf8_next(pos)) {
        w += c32_width(utf8_codepoint(&*pos));
    }
    return w;
}


} // namespace xci::core
