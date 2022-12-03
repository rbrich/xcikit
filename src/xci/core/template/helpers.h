// helpers.h created on 2021-04-19 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_HELPERS_H
#define XCI_CORE_HELPERS_H

namespace xci::core {


// variant visitor helper
// see https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


} // namespace xci::core

#endif // include guard
