// string.cpp created on 2018-03-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "string.h"

#include "parser/unescape.h"

#include <xci/core/log.h>
#include <xci/compat/macros.h>

#include <fmt/format.h>
#include <widechar_width/widechar_width.h>

#ifdef _WIN32
#include <xci/compat/windows.h>  // stringapiset.h / WideCharToMultiByte
#endif

#include <ranges>
#include <locale>
#include <cctype>
#include <cassert>

namespace xci::core {

using std::string_view;
using fmt::format;


bool remove_prefix(std::string& str, const std::string& prefix)
{
    if (!str.starts_with(prefix))
        return false;
    str.erase(0, prefix.size());
    return true;
}


bool remove_suffix(std::string& str, const std::string& suffix)
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
static std::vector<std::string_view> _split(string_view str, TDelim delim, size_t delim_len, int maxsplit)
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

std::vector<std::string_view> split_ws(string_view str, int maxsplit)
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
static std::vector<std::string_view> _rsplit(string_view str, TDelim delim, size_t delim_len, int maxsplit)
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


std::string escape(std::string_view str, bool extended, bool utf8)
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
static std::string gen_unescape(string_view str)
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
    std::ranges::transform(str,
        result.begin(), [](char c){ return (char) std::tolower(c); });
    return result;
}


bool ci_equal(std::string_view s1, std::string_view s2)
{
    return std::ranges::equal(s1, s2,
                      [](char a, char b) { return std::tolower(a) == std::tolower(b); });
}


std::u32string to_utf32(string_view utf8)
{
    std::u32string res;
    res.reserve(utf8.size());
    unsigned pos = 0;
    while (pos < utf8.size()) {
        auto [len, c32] = utf8_codepoint_and_length(utf8.substr(pos));
        if (len != 0) {
            res.push_back(c32);
            pos += len;
        } else {
            // Invalid UTF-8 sequence - surrogate encode, skip 1 byte and try again
            res.push_back(0x0000DC00 + utf8.front());
            ++pos;
        }
    }
    res.shrink_to_fit();
    return res;
}


std::string to_utf8(std::u32string_view u32str)
{
    std::string res;
    res.reserve(u32str.size());
    for (const char32_t c : u32str) {
        res += to_utf8(c);
    }
    return res;
}


#ifdef _WIN32
std::string to_utf8(std::wstring_view wstr)
{
    std::string out;
    out.resize(wstr.size() * 4);
    int r = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstr.size(),
        out.data(), out.size(), nullptr, nullptr);
    if (r == 0) {
        log::error("to_utf8: UTF-16 conversion failed: {m:l}");
        return {};
    }
    out.resize(r);
    out.shrink_to_fit();
    return out;
}
#endif


std::string to_utf8(char32_t codepoint)
{
    if (codepoint < 0x0000007F) {
        return {char(codepoint)};
    }
    if (codepoint < 0x000007FF) {
        return {char(0xC0 | (codepoint >> 6)), char(0x80 | (codepoint & 0x3F))};
    }
    if (codepoint >= 0x0000DC80 && codepoint <= 0x0000DCFF) {
        // Surrogate codes U+DC80..U+DCFF (PEP 383)
        return {char(codepoint - 0x0000DC00)};
    }
    if (codepoint < 0x0000FFFF) {
        return {char(0xE0 | (codepoint >> 12)), char(0x80 | ((codepoint >> 6) & 0x3F)),
                char(0x80 | (codepoint & 0x3F))};
    }
    if (codepoint < 0x001FFFFF) {
        return {char(0xF0 | (codepoint >> 18)), char(0x80 | ((codepoint >> 12) & 0x3F)),
                char(0x80 | ((codepoint >> 6) & 0x3F)), char(0x80 | (codepoint & 0x3F))};
    }
    log::error("to_utf8(codepoint): Invalid unicode codepoint: {:0x}", uint32_t(codepoint));
    return "�";
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
        return char32_t(c0 & 0x7F);
    }
    if ((c0 & 0xE0) == 0xC0) {
        // 110xxxxx -> 2 bytes
        return char32_t(((c0 & 0x1F) << 6) | (utf8[1] & 0x3F));
    }
    if ((c0 & 0xF0) == 0xE0) {
        // 1110xxxx -> 3 bytes
        return char32_t(((c0 & 0x0F) << 12) | ((utf8[1] & 0x3F) << 6) | (utf8[2] & 0x3F));
    }
    if ((c0 & 0xf8) == 0xf0) {
        // 11110xxx -> 4 bytes
        return char32_t(((c0 & 0x07) << 18) | ((utf8[1] & 0x3F) << 12) |
                        ((utf8[2] & 0x3F) << 6) | (utf8[3] & 0x3F));
    }
    log::error("utf8_codepoint: Invalid UTF8 string, encountered code {:02x}", int(c0));
    return 0x0000FFFD;
}


std::pair<int, char32_t> utf8_codepoint_and_length(std::string_view utf8)
{
    const uint32_t size = utf8.size();
    if (size == 0)
        return {0, 0};
    const char c0 = utf8[0];
    if ((c0 & 0x80) == 0) {
        // 0xxxxxxx -> 1 byte
        return {1, char32_t(c0 & 0x7F)};
    }
    if (size == 1)
        return {0, 0};
    const char c1 = utf8[1];
    if ((c1 & 0xC0) != 0x80)
        return {0, -1};
    if ((c0 & 0xe0) == 0xc0) {
        // 110xxxxx -> 2 bytes
        return {2, char32_t(((c0 & 0x1F) << 6) | (c1 & 0x3F))};
    }
    if (size == 2)
        return {0, 0};
    const char c2 = utf8[2];
    if ((c2 & 0xC0) != 0x80)
        return {0, -1};
    if ((c0 & 0xf0) == 0xe0) {
        // 1110xxxx -> 3 bytes
        return {3, char32_t(((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F))};
    }
    if (size == 3)
        return {0, 0};
    const char c3 = utf8[3];
    if ((c3 & 0xC0) != 0x80)
        return {0, -1};
    if ((c0 & 0xf8) == 0xf0) {
        // 11110xxx -> 4 bytes
        return {4, char32_t(((c0 & 0x07) << 18) | ((c1 & 0x3F) << 12) |
                            ((c2 & 0x3F) << 6) | (c3 & 0x3F))};
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
