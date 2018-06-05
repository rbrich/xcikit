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
#include <xci/util/log.h>

#include <cctype>
#include <locale>
#include <codecvt>
#include <cassert>

namespace xci {
namespace util {

std::string escape(const std::string& str)
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
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                case '\v': out += "\\v"; break;
                case '\\': out += "\\\\"; break;
                default: {
                    std::string buf;
                    buf.resize(4);
                    std::snprintf(&buf[0], 4, "\\x%02X", ch);
                    out += buf;
                }
            }
        }
    }
    return out;
}

std::u32string to_utf32(const std::string& utf8)
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert_utf32;
    try {
        return convert_utf32.from_bytes(utf8);
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


std::string::const_iterator
utf8_next(std::string::const_iterator pos)
{
    auto first = (unsigned char) *pos;
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
        assert(!"Invalid UTF8 string");
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


int utf8_length(const std::string& str)
{
    int length = 0;
    auto pos = str.cbegin();
    while (pos != str.cend()) {
        pos = utf8_next(pos);
        ++length;
    }
    return length;
}


}} // namespace xci::log
