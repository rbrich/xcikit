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

#include "format.h"
#include <xci/core/sys.h>

#include <tao/pegtl.hpp>

namespace xci::core::format_impl {

namespace parser {
using namespace tao::pegtl;

// ----------------------------------------------------------------------------
// Grammar

struct FieldName: plus<alnum> {};
struct ZeroFill: one<'0'> {};
struct Width: plus<digit> {};
struct Precision: plus<digit> {};
struct Type: one<'f', 'x', 'X'> {};

// [ZeroFill] [Width] ["." Precision] [Type]
struct FormatSpec: seq<
           opt<ZeroFill>,
           opt<Width>,
           opt<seq< one<'.'>, Precision >>,
           opt<Type>
       > {};

// [FieldName] [":" FormatSpec]
struct Grammar: seq< opt<FieldName>, opt< seq< one<':'>, FormatSpec > > > {};

// ----------------------------------------------------------------------------
// Actions

template<typename Rule>
struct Action : nothing<Rule> {};

template<>
struct Action<FieldName>
{
    template<typename Input>
    static void apply(const Input &in, Context &ctx)
    {
        ctx.field_name = in.string();
    }
};

template<>
struct Action<ZeroFill>
{
    static void apply0(Context &ctx)
    {
        ctx.fill = '0';
    }
};

template<>
struct Action<Width>
{
    template<typename Input>
    static void apply(const Input &in, Context &ctx)
    {
        ctx.width = atoi(in.string().c_str());
    }
};

template<>
struct Action<Precision>
{
    template<typename Input>
    static void apply(const Input &in, Context &ctx)
    {
        ctx.precision = atoi(in.string().c_str());
    }
};

template<>
struct Action<Type>
{
    template<typename Input>
    static void apply(const Input &in, Context &ctx)
    {
        ctx.type = *in.begin();
    }
};

} // namespace parser


bool parse_placeholder(Context& ctx)
{
    using parser::Grammar;
    using parser::Action;

    tao::pegtl::memory_input<> in(ctx.placeholder, "Placeholder");

    try {
        return tao::pegtl::parse< Grammar, Action >( in, ctx );
    } catch (tao::pegtl::parse_error&) {
        return false;
    }
}


bool partial_format(const char*& fmt, Context& ctx)
{
    ctx.clear();
    bool in_placeholder = false;
    while (*fmt) {
        if (in_placeholder) {
            if (*fmt == '}') {
                in_placeholder = false;
                ++fmt;
                if (ctx.placeholder == "m") {
                    // "{m}" -> strerror
                    ctx.stream << errno_str;
                } else if (ctx.placeholder == "mm") {
                    // "{mm}" -> strerror / GetLastError
                    ctx.stream << last_error_str;
                } else {
                    // Leave other placeholders for processing by caller
                    parse_placeholder(ctx);
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


}  // namespace xci::core::format_impl
