// string.h created on 2018-03-23, part of XCI toolkit
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

#ifndef XCI_UTIL_STRING_H
#define XCI_UTIL_STRING_H

#include <string>
#include <xci/compat/string_view.h>

namespace xci {
namespace util {

// Escape non-printable characters with C escape sequences (eg. '\n')
std::string escape(std::string_view str);

// Convert UTF8 string to UTF32, ie. extract Unicode code points.
// In case of invalid source string, logs error and returns empty string.
std::u32string to_utf32(std::string_view utf8);

// Convert single UTF8 character to Unicode code point.
// Only the first UF8 character is used, rest of input is ignored.
// In case of error, return 0.
char32_t to_codepoint(std::string_view utf8);

// Convert single UTF32 char to UTF8 string. Can't fail.
std::string to_utf8(char32_t codepoint);

const char* utf8_next(const char* pos);

std::string::const_reverse_iterator
utf8_prev(std::string::const_reverse_iterator pos);

size_t utf8_length(std::string_view str);

}} // namespace xci::util

#endif // XCI_UTIL_STRING_H
