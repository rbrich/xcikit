// format.h created on 2018-03-01, part of XCI toolkit
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

#ifndef XCI_UTIL_FORMAT_H
#define XCI_UTIL_FORMAT_H

#include <iostream>
#include <sstream>
#include <iomanip>

namespace xci {
namespace util {


// Using variadic templates of C++11, we can implement safe format function,
// which checks argument types and does not require type specifiers in format
// string.
//
// See:
// - https://en.wikipedia.org/wiki/Variadic_template

namespace format_impl {

struct Context {
    std::ostringstream stream;
    std::string placeholder;
    // parsed placeholder:
    std::string field_name;
    int precision;
    char type;

    void clear() {
        placeholder.clear();
        field_name.clear();
        precision = 6;
        type = 's';
    }
};

// Parses `fmt` (moves the pointer), outputs into `stream` and fills `placeholder`.
// Stops on first placeholder it cannot process by itself.
// Returns true if stopped on placeholder, false if reached end of `fmt`.
bool partial_format(const char*& fmt, Context& ctx);

} // namespace format_impl

// Terminates evaluation when there are no more arguments
//
// Note that `fmt` is const char*, not std::string. This has two advantages:
// - We can handle part of it to next recursive iteration
// - It guides the user to pass string literal instead of a variable
inline std::string format(const char *fmt)
{
    format_impl::Context ctx;
    while (*fmt) {
        if (format_impl::partial_format(fmt, ctx)) {
            // unexpected placeholder -> leave as is
            ctx.stream << "{" << ctx.placeholder << "}";
        }
    }
    return ctx.stream.str();
}

// Recursively evaluates arguments
//
// The format string is similar to Python's format(), see:
// - https://docs.python.org/3/library/string.html#formatspec
//
// Only subset of format specifiers is supported
// (see code bellow and tests)

template<typename T, typename ...Args>
inline std::string format(const char *fmt, T value, Args... args)
{
    format_impl::Context ctx;
    while (*fmt) {
        if (format_impl::partial_format(fmt, ctx)) {
            // replace placeholder with arg value
            if (ctx.field_name.empty()) {
                switch (ctx.type) {
                    case 'f': ctx.stream << std::fixed; break;
                    case 'x': ctx.stream << std::hex; break;
                    default: break;
                }
                ctx.stream << std::setprecision(ctx.precision)
                           << value << format(fmt, args...);
                return ctx.stream.str();
            }

            // unknown placeholder -> leave as is
            ctx.stream << "{" << ctx.placeholder << "}";
        }
    }
    return ctx.stream.str();
}


}}  // namespace xci::util

#endif // XCI_UTIL_FORMAT_H
