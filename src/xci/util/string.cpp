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


char32_t to_codepoint(std::string_view utf8)
{
    // TODO: This is very slow. Optimize.
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert_utf32;
    try {
        return convert_utf32.from_bytes(utf8.cbegin(), utf8.cend())[0];
    } catch (const std::range_error& e) {
        log_error("to_codepoint: Invalid UTF8 string: {}", utf8);
        return 0;
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
        log_error("Invalid UTF8 string, encountered code {:02x}", int(first));
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
    int length = 0;
    for (auto pos = str.cbegin(); pos != str.cend(); pos = utf8_next(pos)) {
        ++length;
    }
    return length;
}


}} // namespace xci::log
