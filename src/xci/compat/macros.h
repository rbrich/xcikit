// macros.h created on 2018-11-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_COMPAT_MACROS_H
#define XCI_COMPAT_MACROS_H

#include <utility>

// Unreachable
#if defined(__cpp_lib_unreachable) && __cpp_lib_unreachable >= 202202L
#   define XCI_UNREACHABLE  std::unreachable()
#elif defined(_MSC_VER)
#   define XCI_UNREACHABLE  __assume(0)
#else
#   define XCI_UNREACHABLE  __builtin_unreachable()
#endif


// Force inline / always inline
#ifdef _MSC_VER
#   define XCI_INLINE       __forceinline
#else
#   define XCI_INLINE       inline __attribute__((always_inline))
#endif


// Alternative to [[maybe_unused]] which can be applied to a statement
#define XCI_UNUSED          (void)


// Suppress "deprecated-declarations" warning on an expression
// The parameters should be full declarations, terminated by semicolons
#ifdef __clang__
#define XCI_IGNORE_DEPRECATED(...)                                      \
    _Pragma ("clang diagnostic push");                                  \
    _Pragma ("clang diagnostic ignored \"-Wdeprecated-declarations\""); \
    __VA_ARGS__                                                         \
    _Pragma ("clang diagnostic pop");
#else
#define XCI_IGNORE_DEPRECATED(...)  __VA_ARGS__
#endif


#endif // include guard
