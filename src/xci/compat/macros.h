// macros.h created on 2018-11-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_COMPAT_MACROS_H
#define XCI_COMPAT_MACROS_H


#if __cplusplus >= 201703L || __has_cpp_attribute(fallthrough)
    #define FALLTHROUGH [[fallthrough]]
#else
    #define FALLTHROUGH
#endif


#ifdef _MSC_VER
    #define UNREACHABLE     __assume(0)
#else
    #define UNREACHABLE     __builtin_unreachable()
#endif


// because [[maybe_unused]] can't be applied to a statement
#define UNUSED      (void)

#endif // include guard
