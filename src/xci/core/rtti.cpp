// rtti.cpp created on 2018-07-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "rtti.h"

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#endif

#include <cstdlib>  // free

namespace xci::core {


std::string demangle(const char* name)
{
#if defined(__GNUC__) || defined(__clang__)
    int status;
    char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    if (status != 0) {
        // failure
        return name;
    }
    std::string result(demangled);
    std::free(demangled);
    return result;
#else
    return name;
#endif
}


std::string demangle_type_name(const char* name)
{
#if defined(__GNUC__) || defined(__clang__)
    return demangle(name);
#else
    // Windows: not mangled
    std::string res(name);
    for (const char* prefix : {"struct ", "class "}) {
        if (res.starts_with(prefix))
            res.erase(0, strlen(prefix));
    }
    return res;
#endif
}


}  // namespace xci::core
