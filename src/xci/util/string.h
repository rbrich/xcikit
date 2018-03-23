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

namespace xci {
namespace util {

// Escape non-printable characters with C escape sequences (eg. '\n')
std::string escape(const std::string& str);

// Convert UTF8 string to UTF32, ie. extract Unicode code points.
// In case of invalid source string, logs error and returns empty string.
std::u32string to_utf32(const std::string& utf8);

}} // namespace xci::log

#endif // XCI_UTIL_STRING_H
