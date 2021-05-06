// unescape.h created on 2019-11-29 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_PARSER_UNESCAPE_H
#define XCI_CORE_PARSER_UNESCAPE_H

#include "unescape_rules.h"

namespace xci::core::parser::unescape {

// This is tolerant version of the unescape parser,
// consuming any output and interpreting unknown escapes in best effort manner.
struct StringChIll : any {};
struct StringChIllEsc : seq< one< '\\' >, sor<StringChIll, eof> > {};
struct String: star< sor<try_catch<StringCh>, StringChIllEsc, StringChIll> > {};

template< typename Rule > struct Action {};
template<> struct Action<StringChOther> : StringAppendChar {};
template<> struct Action<StringChIll> : StringAppendChar {};
template<> struct Action<StringChEscSingle> : StringAppendEscSingle {};
template<> struct Action<StringChEscHex> : StringAppendEscHex {};
template<> struct Action<StringChEscOct> : StringAppendEscOct {};

} // namespace xci::core::parser

#endif // include guard
