// rtti.h created on 2018-07-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_RTTI_H
#define XCI_CORE_RTTI_H

#include <string>
#include <typeinfo>

namespace xci::core {


// Demangle type name when mangled (GCC/Clang)
std::string demangle_type_name(const char* name);

// Returns human-readable type name for given typeid()
// Typical usage: `type_name(typeid(*this))`
inline std::string type_name(const std::type_info& ti) {
    return demangle_type_name(ti.name());
}


}  // namespace xci::core

#endif // XCI_CORE_RTTI_H
