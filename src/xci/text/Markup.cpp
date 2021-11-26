// Markup.cpp created on 2018-03-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Markup.h"
#include <xci/config.h>  // XCI_DEBUG_MARKUP_DUMP_TOKENS
#include <xci/core/log.h>
#include <xci/core/string.h>

#include <tao/pegtl.hpp>
#include <fmt/ostream.h>

namespace xci::text {

using namespace xci::core;

namespace parser {

using namespace tao::pegtl;

// ----------------------------------------------------------------------------
// Grammar

struct ControlSeq: seq< one<'<'>, plus<not_one<'<', '>'>>, one<'>'> > {};

struct Word: plus<not_at<space>, sor< not_one<'<'>, two<'<'> >> {};

struct Paragraph: two<'\n'> {};

struct Tab: one<'\t'> {};

struct Space: plus<space> {};

struct Grammar: must< star<sor< ControlSeq, Word, Paragraph, Tab, Space >>, eof > {};

// ----------------------------------------------------------------------------
// Actions

template<typename T>
void dump_token(const char *name, T& token)
{
#ifdef XCI_DEBUG_MARKUP_DUMP_TOKENS
    log::debug("{}: \"{}\"", name, core::escape(token.string()).c_str());
#endif
}

template<typename Rule>
struct Action : nothing<Rule> {};

template<>
struct Action<ControlSeq>
{
    template<typename Input>
    static void apply(const Input &in, Markup &ctx)
    {
        dump_token("csq", in);
        auto seq = in.string();
        if (seq == "<tab>")
            return ctx.get_layout().add_tab();
        if (seq == "<br>")
            return ctx.get_layout().finish_line();
        if (seq == "<p>")
            return ctx.get_layout().advance_line(0.5f);
        if (seq == "<b>")
            return ctx.get_layout().set_bold();
        if (seq == "</b>")
            return ctx.get_layout().set_bold(false);
        if (seq == "<i>")
            return ctx.get_layout().set_italic();
        if (seq == "</i>")
            return ctx.get_layout().set_italic(false);
        if (seq[1] == '/') {
            auto name = seq.substr(2, seq.size() - 3);
            ctx.get_layout().end_span(name);
        } else {
            auto name = seq.substr(1, seq.size() - 2);
            ctx.get_layout().begin_span(name);
        }
    }
};

template<>
struct Action<Word>
{
    template<typename Input>
    static void apply(const Input &in, Markup &ctx)
    {
        dump_token("word", in);
        ctx.get_layout().add_word(in.string());
    }
};

template<>
struct Action<Tab>
{
    template<typename Input>
    static void apply(const Input &in, Markup &ctx)
    {
        dump_token("tab", in);
        ctx.get_layout().add_tab();
    }
};

template<>
struct Action<Paragraph>
{
    template<typename Input>
    static void apply(const Input &in, Markup &ctx)
    {
        dump_token("par", in);
        ctx.get_layout().advance_line(0.5f);
    }
};

template<>
struct Action<Space>
{
    template<typename Input>
    static void apply(const Input &in, Markup &ctx)
    {
        dump_token("space", in);
        ctx.get_layout().add_space();
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
        log::error("{}: Parse error matching {} at [{}]",
                  in.position(),
                  demangle<Rule>(),
                  std::string(in.current(), in.size()).substr(0, 10));
        throw parse_error( "parse error matching " + std::string(demangle<Rule>()), in );
    }
};

} // namespace parser


bool Markup::parse(const std::string &s)
{
    using parser::Grammar;
    using parser::Action;
    using parser::Control;

    tao::pegtl::memory_input<> in(s, "Markup");

    try {
        return tao::pegtl::parse< Grammar, Action, Control >( in, *this );
    } catch (const tao::pegtl::parse_error&) {
        return false;
    }
}


} // namespace xci::text
