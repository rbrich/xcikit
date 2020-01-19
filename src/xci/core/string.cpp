// string.cpp created on 2018-03-23 belongs to XCI Toolkit
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "string.h"
#include "parser/unescape.h"
#include <xci/core/format.h>
#include <xci/core/log.h>

#include <cctype>
#include <locale>
#include <codecvt>
#include <cassert>

namespace xci::core {

using namespace std;


bool starts_with(const std::string& str, const std::string& sub)
{
    // Note: don't use find(), that would be ineffective to decide negative result
    return str.size() >= sub.size() && str.substr(0, sub.size()) == sub;
}


std::vector<string_view> split(string_view str, char delim)
{
    std::vector<string_view> res;
    size_t pos = 0;
    size_t end = 0;
    for (;;) {
        end = str.find(delim, pos);
        if (end != string_view::npos) {
            if (end != pos)
                res.push_back(str.substr(pos, end - pos));
            pos = end + 1;
        } else {
            if (pos < str.size() - 1)
                res.push_back(str.substr(pos, end - pos));
            break;
        }
    }
    return res;
}


std::string escape(string_view str)
{
    std::string out;
    out.reserve(str.size());
    for (auto ch : str) {
        switch (ch) {
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
                if (std::isprint(ch))
                    out += ch;
                else if (ch >= 0 && ch < 8)
                    out += format("\\{}", (int)(unsigned char)(ch));
                else
                    out += format("\\x{:02x}", (int)(unsigned char)(ch));
                break;
            }
        }
    }
    return out;
}


std::string unescape(string_view str)
{
    using namespace parser::unescape;

    tao::pegtl::memory_input<
        tao::pegtl::tracking_mode::eager,
        tao::pegtl::eol::lf_crlf,
        const char*>
    input(str.data(), str.size(), "<input>");
    std::string result;
    result.reserve(str.size());

    try {
        auto matched = tao::pegtl::parse< String, Action >( input, result );
        assert(matched);
        (void) matched;
    } catch (tao::pegtl::parse_error&) {
        assert(!"unescape: parse error");
    }
    result.shrink_to_fit();
    return result;
}


std::string to_lower(std::string_view str)
{
    std::string result(str.size(), '\0');
    std::transform(
        str.begin(), str.end(),
        result.begin(), [](char c){ return std::tolower(c); });
    return result;
}


std::u32string to_utf32(string_view utf8)
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert_utf32;
    try {
        return convert_utf32.from_bytes(utf8.cbegin(), utf8.cend());
    } catch (const std::range_error& e) {
        log_error("to_utf32: Invalid UTF8 string: {}", utf8);
        return std::u32string();
    }
}


std::string to_utf8(char32_t codepoint)
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert_utf32;
    return convert_utf32.to_bytes(codepoint);
}


template <class I>
I utf8_next(I iter)
{
    auto first = (unsigned char) *iter;
    if (first == 0) {
        return iter;
    } else
    if ((first & 0b10000000) == 0) {
        // 0xxxxxxx -> 1 byte
        return iter + 1;
    } else
    if ((first & 0b11100000) == 0b11000000) {
        // 110xxxxx -> 2 bytes
        return iter + 2;
    } else
    if ((first & 0b11110000) == 0b11100000) {
        // 1110xxxx -> 3 bytes
        return iter + 3;
    } else
    if ((first & 0b11111000) == 0b11110000) {
        // 11110xxx -> 4 bytes
        return iter + 4;
    } else {
        //log_error("utf8_next: Invalid UTF8 string, encountered code 0x{:02x}", int(first));
        return iter + 1;
    }
}

// instantiate the template (these are the only supported types)
template std::string::const_iterator utf8_next(std::string::const_iterator);
template std::string_view::const_iterator utf8_next(std::string_view::const_iterator);


template <class I>
I utf8_prev(I riter)
{
    while ((*riter & 0b11000000) == 0b10000000)
        ++riter;
    return riter + 1;
}

// instantiate the template (these are the only supported types)
template std::string::const_reverse_iterator utf8_prev(std::string::const_reverse_iterator);
template std::string_view::const_reverse_iterator utf8_prev(std::string_view::const_reverse_iterator);


template <class S, class SSize>
SSize utf8_length(const S& str)
{
    SSize length = 0;
    for (auto pos = str.cbegin(); pos != str.cend(); pos = utf8_next(pos)) {
        ++length;
    }
    return length;
}

// instantiate the template (these are the only supported types)
template std::string::size_type utf8_length<std::string>(const std::string&);
template std::string_view::size_type utf8_length<std::string_view>(const std::string_view&);


string_view utf8_substr(string_view str, size_t pos, size_t count)
{
    auto begin = str.cbegin();
    while (pos > 0 && begin != str.cend()) {
        begin = utf8_next(begin);
        --pos;
    }
    auto end = begin;
    while (count > 0 && end != str.cend()) {
        end = utf8_next(end);
        --count;
    }
    return {begin, size_t(end - begin)};
}


char32_t utf8_codepoint(const char* utf8)
{
    char c0 = utf8[0];
    if ((c0 & 0x80) == 0) {
        // 0xxxxxxx -> 1 byte
        return char32_t(c0 & 0x7f);
    } else
    if ((c0 & 0xe0) == 0xc0) {
        // 110xxxxx -> 2 bytes
        return char32_t(((c0 & 0x1f) << 6) | (utf8[1] & 0x3f));
    } else
    if ((c0 & 0xf0) == 0xe0) {
        // 1110xxxx -> 3 bytes
        return char32_t(((c0 & 0x0f) << 12) | ((utf8[1] & 0x3f) << 6) | (utf8[2] & 0x3f));
    } else
    if ((c0 & 0xf8) == 0xf0) {
        // 11110xxx -> 4 bytes
        return char32_t(((c0 & 0x07) << 18) | ((utf8[1] & 0x3f) << 12) | ((utf8[2] & 0x3f) << 6) | (utf8[3] & 0x3f));
    } else {
        log_error("utf8_codepoint: Invalid UTF8 string, encountered code {:02x}", int(c0));
        return 0;
    }
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


} // namespace xci::core
