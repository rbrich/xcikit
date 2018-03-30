// format.cpp created on 2018-03-30, part of XCI toolkit
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

#include <cstring>
#include "format.h"

namespace xci {
namespace util {
namespace format_impl {


std::ostream& strerror(std::ostream& stream)
{
    char buf[100] = {};
#if (_POSIX_C_SOURCE >= 200112L) && !_GNU_SOURCE
    if (strerror_r(errno, buf, sizeof buf) == 0) {
        stream << buf;
    }
    return stream;
#else
    return stream << strerror_r(errno, buf, sizeof buf);
#endif
}


bool partial_format(const char*& fmt, Context& ctx)
{
    ctx.placeholder.clear();
    bool in_placeholder = false;
    while (*fmt) {
        if (in_placeholder) {
            if (*fmt == '}') {
                in_placeholder = false;
                ++fmt;
                if (ctx.placeholder == ":m") {
                    // "{:m}" -> strerror
                    ctx.stream << strerror;
                } else {
                    // Leave other placeholders for processing by caller
                    return true;
                }
                ctx.placeholder.clear();
            } else {
                ctx.placeholder += *fmt++;
            }
            continue;
        }
        if (*fmt == '{') {
            // "{{" -> "{"
            if (*(fmt + 1) == '{') {
                ++fmt;
            } else
            // "{" -> start reading placeholder
            {
                in_placeholder = true;
                ++fmt;
                continue;
            }
        }
        ctx.stream << *fmt++;
    }
    return false;
}


}}}  // namespace xci::util::format_impl
