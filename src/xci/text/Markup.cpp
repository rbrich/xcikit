// Markup.cpp created on 2018-03-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
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

struct OpenElem: plus<not_one<'<', '>'>> {};
struct CloseElem: plus<not_one<'<', '>'>> {};

struct Tag: seq< one<'<'>, sor<seq<one<'/'>, CloseElem>, OpenElem>, one<'>'> > {};

struct Word: plus<not_at<space>, sor< not_one<'<'>, two<'<'> >> {};

struct Paragraph: two<'\n'> {};

struct Tab: one<'\t'> {};

struct Space: plus<space> {};

struct Grammar: must< star<sor< Tag, Word, Paragraph, Tab, Space >>, eof > {};

// ----------------------------------------------------------------------------
// Actions

template<typename T>
void dump_token(const char *name, T& token)
{
#ifdef XCI_DEBUG_MARKUP_DUMP_TOKENS
    log::debug("{}: \"{}\"", name, core::escape(token.string()));
#endif
}

template<typename Rule>
struct Action : nothing<Rule> {};

template<>
struct Action<OpenElem>
{
    template<typename Input>
    static void apply(const Input &in, Markup &ctx)
    {
        dump_token("csq", in);
        auto seq = in.string();
        if (seq == "tab")
            return ctx.get_layout().add_tab();
        if (seq == "br")
            return ctx.get_layout().new_line();
        if (seq == "p")
            return ctx.get_layout().new_line(1.5f);
        if (seq == "b")
            return ctx.get_layout().set_bold();
        if (seq == "i")
            return ctx.get_layout().set_italic();
        if (seq.starts_with("c:")) {
            return ctx.get_layout().set_color(graphics::Color(seq.substr(2)));
        }
        if (seq.starts_with("s:")) {
            auto name = seq.substr(2);
            return ctx.get_layout().begin_span(name);
        }
        // Unknown tag - leave uninterpreted
        ctx.get_layout().add_word(std::string("<") + seq + ">");
    }
};

template<>
struct Action<CloseElem>
{
    template<typename Input>
    static void apply(const Input &in, Markup &ctx)
    {
        dump_token("end", in);
        auto seq = in.string();
        if (seq == "b")
            return ctx.get_layout().set_bold(false);
        if (seq == "i")
            return ctx.get_layout().set_italic(false);
        if (seq == "c" || seq.starts_with("c:")) {
            return ctx.get_layout().reset_color();
        }
        if (seq.starts_with("s:")) {
            auto name = seq.substr(2);
            return ctx.get_layout().end_span(name);
        }
        // Unknown tag - leave uninterpreted
        ctx.get_layout().add_word(std::string("</") + seq + ">");
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
        ctx.get_layout().new_line(1.5f);
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
                  fmt::streamed(in.position()),
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


void parse_plain(Layout& layout, const std::string& s)
{
    auto it = s.begin();
    auto word = s.end();
    auto finish_word = [&] {
        if (word == s.end())
            return;
        layout.add_word(std::string{word, it});
        word = s.end();
    };
    while (it != s.end()) {
        switch (*it) {
            case '\t':
                finish_word();
                layout.add_tab();
                break;
            case '\n':
                finish_word();
                layout.new_line();
                break;
            default:
                if (word == s.end())
                    word = it;
                break;
        }
        ++it;
    }
    finish_word();
}


} // namespace xci::text
