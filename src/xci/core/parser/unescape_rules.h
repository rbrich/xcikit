// unescape_rules.h created on 2019-11-29 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_PARSER_UNESCAPE_RULES_H
#define XCI_CORE_PARSER_UNESCAPE_RULES_H

#include <xci/core/string.h>
#include <tao/pegtl.hpp>

namespace xci::core::parser::unescape {

using namespace tao::pegtl;


struct StringChEscSingle : one< 'a', 'b', 'f', 'n', 'r', 't', 'v', 'e', '\\', '"', '\'', '\n' > {};  // \a, ...
struct StringChEscOct : seq< odigit, opt<odigit>, opt<odigit> > {};          //  \0, \377
struct StringChEscHex : if_must< one<'x'>, xdigit, xdigit > {};              //  \xFF
struct StringChEscUni : if_must< one<'u'>, one<'{'>, rep_min_max<1,6,xdigit>, one<'}'> > {};  //  \u{10234F}
struct StringChEscSpec : sor< StringChEscHex, StringChEscOct, StringChEscSingle > {};
struct StringChEsc : if_must< one<'\\'>, StringChEscSpec > {};
struct StringChUniEscSpec : sor< StringChEscHex, StringChEscUni, StringChEscOct, StringChEscSingle > {};
struct StringChUniEsc : if_must< one<'\\'>, StringChUniEscSpec > {};
struct StringChOther : sor< one<'\t'>, utf8::not_range<0, 31> > {};
struct StringCh : sor< StringChEsc, StringChOther > {};
struct StringChUni : sor< StringChUniEsc, StringChOther > {};


struct StringAppendChar {
    template<typename Input, typename String, typename... States>
    static void apply(const Input &in, String& str, States&&...) {
        str.append(in.begin(), in.end());
    }
};


struct StringAppendEscSingle {
    template<typename Input, typename String, typename... States>
    static void apply(const Input &in, String& str, States&&...) {
        assert( in.size() == 1 );
        char ch = *in.begin();
        switch (ch) {
            case 'a': str += '\a'; break;
            case 'b': str += '\b'; break;
            case 'f': str += '\f'; break;
            case 'n': str += '\n'; break;
            case 'r': str += '\r'; break;
            case 't': str += '\t'; break;
            case 'v': str += '\v'; break;
            case 'e': str += '\033'; break;
            case '\n': break;
            default: str += ch; break;
        }
    }
};


struct StringAppendEscOct {
    template<typename Input, typename String, typename... States>
    static void apply(const Input &in, String& str, States&&...) {
        assert( in.size() >= 1 && in.size() <= 3 );
        str += (char) std::stoi(in.string(), nullptr, 8);
    }
};


struct StringAppendEscHex {
    template<typename Input, typename String, typename... States>
    static void apply(const Input &in, String& str, States&&...) {
        assert( in.size() == 3 );
        str += (char) std::stoi(String{in.begin()+1, in.end()}, nullptr, 16);
    }
};


struct StringAppendEscUni {
    template<typename Input, typename String, typename... States>
    static void apply(const Input &in, String& str, States&&...) {
        assert( in.size() > 3 && in.size() <= 9 );
        str += to_utf8( (char32_t) std::stoi(String{in.begin()+2, in.end()-1}, nullptr, 16) );
    }
};


} // namespace xci::core::parser::unescape

#endif // include guard
