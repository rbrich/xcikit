// string.cpp created on 2018-03-23, part of XCI toolkit
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <xci/util/string.h>
#include <xci/util/format.h>
#include <xci/util/log.h>

#include <cctype>
#include <locale>
#include <codecvt>
#include <cassert>
#include "string.h"


namespace xci {
namespace util {

std::string escape(std::string_view str)
{
    std::string out;
    out.reserve(str.size());
    for (auto ch : str) {
        if (std::isprint(ch)) {
            out += ch;
        } else {
            switch (ch) {
                case '\a': out += "\\a"; break;
                case '\b': out += "\\b"; break;
                case '\t': out += "\\t"; break;
                case '\n': out += "\\n"; break;
                case '\v': out += "\\v"; break;
                case '\f': out += "\\f"; break;
                case '\r': out += "\\r"; break;
                case '\\': out += "\\\\"; break;
                default: {
                    out += format("\\x{:02x}", (int)(unsigned char)(ch));
                    break;
                }
            }
        }
    }
    return out;
}

std::u32string to_utf32(std::string_view utf8)
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


const char* utf8_next(const char* pos)
{
    auto first = (unsigned char) *pos;
    if (first == 0) {
        return pos;
    } else
    if ((first & 0b10000000) == 0) {
        // 0xxxxxxx -> 1 byte
        return pos + 1;
    } else
    if ((first & 0b11100000) == 0b11000000) {
        // 110xxxxx -> 2 bytes
        return pos + 2;
    } else
    if ((first & 0b11110000) == 0b11100000) {
        // 1110xxxx -> 3 bytes
        return pos + 3;
    } else
    if ((first & 0b11111000) == 0b11110000) {
        // 11110xxx -> 4 bytes
        return pos + 4;
    } else {
        log_error("utf8_next: Invalid UTF8 string, encountered code 0x{:02x}", int(first));
        return pos + 1;
    }
}


std::string::const_reverse_iterator
utf8_prev(std::string::const_reverse_iterator pos)
{
    while ((*pos & 0b11000000) == 0b10000000)
        ++pos;
    return pos + 1;
}


size_t utf8_length(std::string_view str)
{
    size_t length = 0;
    for (auto pos = str.cbegin(); pos != str.cend(); pos = utf8_next(pos)) {
        ++length;
    }
    return length;
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


size_t utf8_partial_end(std::string_view str)
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


}} // namespace xci::log
