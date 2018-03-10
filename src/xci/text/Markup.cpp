// Markup.cpp created on 2018-03-10, part of XCI toolkit
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

#include "Markup.h"

#include <tao/pegtl.hpp>
#include <iostream>

namespace xci {
namespace text {

namespace parser {
using namespace tao::pegtl;

// ----------------------------------------------------------------------------
// Grammar

struct ControlSeq : seq< one<'{'>, plus<not_one<'{', '}'>>, one<'}'> > {};

struct Word: plus<not_at<space>, sor< not_one<'{'>, two<'{'> >> {};

struct Paragraph: two<'\n'> {};

struct Tab: one<'\t'> {};

struct Space: plus<space> {};

struct Grammar : must< star<sor< ControlSeq, Word, Paragraph, Tab, Space >>, eof > {};

// ----------------------------------------------------------------------------
// Actions

template<typename T>
void dump_token(const char *name, T& token)
{
#ifdef MARKUP_DUMP_TOKENS
    printf("%s: \"%s\"\n", name, token.string().c_str());
#endif
}

template<typename Rule>
struct Action : nothing<Rule> {};

template<>
struct Action<ControlSeq>
{
    template<typename Input>
    static bool apply(const Input &in, Markup &ctx)
    {
        dump_token("csq", in);
        return true;
    }
};

template<>
struct Action<Word>
{
    template<typename Input>
    static bool apply(const Input &in, Markup &ctx)
    {
        dump_token("word", in);
        ctx.get_layout().add_word(in.string());
        return true;
    }
};

template<>
struct Action<Tab>
{
    template<typename Input>
    static bool apply(const Input &in, Markup &ctx)
    {
        dump_token("tab", in);
        ctx.get_layout().add_tab();
        return true;
    }
};

template<>
struct Action<Paragraph>
{
    template<typename Input>
    static bool apply(const Input &in, Markup &ctx)
    {
        dump_token("par", in);
        ctx.get_layout().finish_line();
        return true;
    }
};

template<>
struct Action<Space>
{
    template<typename Input>
    static bool apply(const Input &in, Markup &ctx)
    {
        dump_token("space", in);
        ctx.get_layout().add_space();
        return true;
    }
};

// ----------------------------------------------------------------------------
// Control (error reporting)

template< typename Rule >
struct Control : normal< Rule >
{
    template< typename Input, typename... States >
    static void raise( const Input& in, States&&... /*unused*/ )
    {
        std::cerr << in.position()
                  << ": Parse error matching " << internal::demangle< Rule >()
                  << " at [" << std::string(in.current(), in.size()).substr(0, 10) << "]"
                  << std::endl;
        throw parse_error( "parse error matching " + internal::demangle< Rule >(), in );
    }
};

} // namespace parser


bool Markup::parse(const std::string &s)
{
    using parser::Grammar;
    using parser::Action;
    using parser::Control;

    tao::pegtl::memory_input<> in(s, "Markup");

    return tao::pegtl::parse< Grammar, Action, Control >( in, *this );
}


}} // namespace xci::text
