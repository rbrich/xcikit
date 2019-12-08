// unescape_rules.h created on 2019-11-29 belongs to XCI Toolkit
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_PARSER_UNESCAPE_RULES_H
#define XCI_CORE_PARSER_UNESCAPE_RULES_H

#include <tao/pegtl.hpp>

namespace xci::core::parser::unescape {

using namespace tao::pegtl;


struct StringChEscSingle : one< 'a', 'b', 'f', 'n', 'r', 't', 'v', 'e', '\\', '"', '\'', '0', '\n' > {};
struct StringChEscHex : if_must< one< 'x' >, xdigit, xdigit > {};
struct StringChEscOct : if_must< digit, opt<digit>, opt<digit> > {};
struct StringChEsc : if_must< one< '\\' >, sor< StringChEscHex, StringChEscOct, StringChEscSingle > > {};
struct StringChOther : sor< one<'\t'>, not_range<0, 31> > {};
struct StringCh : sor< StringChEsc, StringChOther > {};


struct StringAppend {
    template<typename Input, typename String, typename... States>
    static void apply(const Input &in, String& str, States&&...) {
        if (str.capacity() == str.size())
            str.reserve(str.capacity() * 3 / 2);
        str.append(in.begin(), in.end());
    }
};


struct StringAppendEscSingle {
    template<typename Input, typename String, typename... States>
    static void apply(const Input &in, String& str, States&&...) {
        if (str.capacity() == str.size())
            str.reserve(str.capacity() * 3 / 2);
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


struct StringAppendEscHex {
    template<typename Input, typename String, typename... States>
    static void apply(const Input &in, String& str, States&&...) {
        if (str.capacity() == str.size())
            str.reserve(str.capacity() * 3 / 2);
        assert( in.size() == 3 );
        str += (char) std::stoi(String{in.begin()+1, in.end()}, nullptr, 16);
    }
};


struct StringAppendEscOct {
    template<typename Input, typename String, typename... States>
    static void apply(const Input &in, String& str, States&&...) {
        if (str.capacity() == str.size())
            str.reserve(str.capacity() * 3 / 2);
        assert( in.size() >= 1 && in.size() <= 3 );
        str += (char) std::stoi(in.string(), nullptr, 8);
    }
};


} // namespace xci::core::parser::unescape

#endif // include guard
