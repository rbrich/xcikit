// ConfigParser.cpp created on 2023-11-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ConfigParser.h"
#include <xci/core/log.h>
#include <xci/core/parser/unescape_rules.h>

#include <tao/pegtl.hpp>
#include <fmt/ostream.h>

namespace xci::config {

using namespace xci::core;

namespace parser {

using namespace tao::pegtl;
using namespace xci::core::parser::unescape;

// ----------------------------------------------------------------------------
// Grammar

// Spaces and comments
struct LineComment: seq< two<'/'>, until<eolf> > {};
struct SkipWS: star<sor<space, LineComment>> {};
struct SemicolonOrNewline: sor<one<';'>, eolf, LineComment> {};
struct Sep: seq<star<blank>, SemicolonOrNewline> {};

// Keywords
struct KFalse: TAO_PEGTL_KEYWORD("false") {};
struct KTrue: TAO_PEGTL_KEYWORD("true") {};

// Literals
template<class D> struct UPlus: seq< D, star<opt<one<'_'>>, D> > {};  // one or more digits D interleaved by underscores ('_')
struct Sign: one<'-','+'> {};
struct DecNumFrac: seq< one<'.'>, opt<UPlus<digit>> > {};
struct DecNumExp: seq< one<'e'>, opt<Sign>, must<UPlus<digit>> > {};

// Values
struct Bool: sor<KFalse, KTrue> {};
struct Number: seq<opt<Sign>, UPlus<digit>, opt<DecNumFrac>, opt<DecNumExp>> {};
struct StringContent: until< one<'"'>, StringChUni > {};
struct String: if_must< one<'"'>, StringContent > {};

// Group
struct GroupContent;
struct Group: seq<one<'{'>, GroupContent, SkipWS, one<'}'>> {};

// Item
struct Name: identifier {};
struct Value: sor<Bool, Number, String, Group> {};
struct Item: seq<SkipWS, Name, plus<blank>, Value, Sep> {};

// File
struct GroupContent: star<Item> {};
struct FileContent: seq<GroupContent, SkipWS, eof> {};


// ----------------------------------------------------------------------------
// Actions

template<typename Rule>
struct Action : nothing<Rule> {};

template<>
struct Action<Name> {
    template<typename Input>
    static void apply(const Input& in, ConfigParser& visitor) {
        visitor.name(in.string());
    }
};

template<>
struct Action<Bool> {
    template<typename Input>
    static void apply(const Input& in, ConfigParser& visitor) {
        visitor.bool_value(in.string() == "true");
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


bool ConfigParser::parse_string(const std::string &str)
{
    using parser::FileContent;
    using parser::Action;
    using parser::Control;

    tao::pegtl::memory_input<> in(str, "<buffer>");

    try {
        return tao::pegtl::parse< FileContent, Action, Control >( in, *this );
    } catch (const tao::pegtl::parse_error&) {
        return false;
    }
}


}  // namespace xci::config
