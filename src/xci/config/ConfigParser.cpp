// ConfigParser.cpp created on 2023-11-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ConfigParser.h"
#include <xci/core/log.h>
#include <xci/core/parser/raw_string.h>
#include <xci/core/parser/unescape_rules.h>

#include <tao/pegtl.hpp>

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
struct SemicolonOrNewline: sor<one<';'>, eolf, LineComment, at<one<'}'>>> {};
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
struct String: if_must< one<'"'>, until< one<'"'>, StringChUni > > {};
struct EscapedQuotes: seq<one<'\\'>, three<'"'>, star<one<'"'>>> {};
struct RawStringCh: any {};
struct RawStringContent: until<three<'"'>, sor<EscapedQuotes, RawStringCh>> {};
struct RawString : if_must< three<'"'>, RawStringContent > {};

// Group
struct GroupContent;
struct GroupBegin: one<'{'> {};
struct GroupEnd: one<'}'> {};
struct Group: seq<GroupBegin, GroupContent, SkipWS, GroupEnd> {};

// Item
struct Name: identifier {};
struct Value: sor<Bool, Number, RawString, String, Group> {};
struct Item: seq<SkipWS, Name, plus<blank>, must<Value>, Sep> {};

// File
struct GroupContent: star<Item> {};
struct FileContent: seq<GroupContent, SkipWS, must<eof>> {};


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


struct NumberHelper {
    union {
        double f;
        int64_t i;
    };
    bool is_float = false;
};

template<>
struct Action<Number> : change_states<NumberHelper> {
    template<typename Input>
    static void apply(const Input& in, NumberHelper& n) {
        std::istringstream is(in.string());
        if (n.is_float) {
            is >> n.f;
        } else {
            is >> n.i;
        }
        if (!is.eof()) {
            throw parse_error("Number not fully parsed.", in);
        }
    }

    template<typename Input>
    static void success(const Input& in, NumberHelper& n, ConfigParser& visitor) {
        if (n.is_float) {
            visitor.float_value(n.f);
        } else {
            visitor.int_value(n.i);
        }
    }
};

template<>
struct Action<DecNumFrac> {
    template<typename Input>
    static void apply(const Input&, NumberHelper& n) {
        n.is_float = true;
    }
};

template<>
struct Action<DecNumExp> {
    template<typename Input>
    static void apply(const Input&, NumberHelper& n) {
        n.is_float = true;
    }
};


template<> struct Action<StringChOther> : StringAppendChar {};
template<> struct Action<StringChEscSingle> : StringAppendEscSingle {};
template<> struct Action<StringChEscHex> : StringAppendEscHex {};
template<> struct Action<StringChEscOct> : StringAppendEscOct {};

template<>
struct Action<String> : change_states< std::string > {
    template<typename Input>
    static void success(const Input &in, std::string& str, ConfigParser& visitor) {
        visitor.string_value(std::move(str));
    }
};


template<> struct Action<RawStringCh> : StringAppendChar {};

template<>
struct Action<EscapedQuotes> {
    template<typename Input>
    static void apply(const Input &in, std::string& str) {
        assert(in.size() >= 4);
        str.append(in.begin() + 1, in.end());
    }
};

template<>
struct Action<RawString> : change_states< std::string > {
    template<typename Input>
    static void success(const Input &in, std::string& str, ConfigParser& visitor) {
        visitor.string_value(core::parser::strip_raw_string(std::move(str)));
    }
};


template<>
struct Action<GroupBegin> {
    template<typename Input>
    static void apply(const Input& in, ConfigParser& visitor) {
        visitor.begin_group();
    }
};

template<>
struct Action<GroupEnd> {
    template<typename Input>
    static void apply(const Input& in, ConfigParser& visitor) {
        visitor.end_group();
    }
};


// ----------------------------------------------------------------------------
// Control (error reporting)

template< typename Rule >
struct Control : normal< Rule >
{
    static const char* errmsg;

    template< typename Input, typename... States >
    static void raise( const Input& in, States&&... /*unused*/ ) {
        if (errmsg == nullptr) {
            // default message
#ifndef NDEBUG
            throw parse_error( "parse error matching " + std::string(demangle<Rule>()), in );
#else
            throw parse_error( "parse error", in );
#endif
        }
        throw tao::pegtl::parse_error( errmsg, in );
    }
};

template<> const char* Control<eof>::errmsg = "invalid syntax";
template<> const char* Control<Value>::errmsg = "expected value";

// default message
template<typename T> const char* Control<T>::errmsg = nullptr;

} // namespace parser


template<class T>
static bool _parse(ConfigParser& config_parser, T&& in)
{
    using parser::FileContent;
    using parser::Action;
    using parser::Control;
    try {
        return tao::pegtl::parse< FileContent, Action, Control >( in, config_parser );
    } catch (const tao::pegtl::parse_error& e) {
        const auto& p = e.positions().front();
        log::error("{}\n{}\n{:>{}}", e.what(), in.line_at(p), '^', p.column);
        return false;
    }
}


bool ConfigParser::parse_file(const fs::path& path)
{
    tao::pegtl::file_input in(path);
    return _parse(*this, in);
}


bool ConfigParser::parse_string(std::string_view str)
{
    tao::pegtl::memory_input in(str, "<buffer>");
    return _parse(*this, in);
}


}  // namespace xci::config
