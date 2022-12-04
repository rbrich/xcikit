// macros.h created on 2018-11-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_COMPAT_MACROS_H
#define XCI_COMPAT_MACROS_H


// Force inline / always inline
#ifdef _MSC_VER
#   define XCI_UNREACHABLE  __assume(0)
#   define XCI_INLINE       __forceinline
#else
#   define XCI_UNREACHABLE  __builtin_unreachable()
#   define XCI_INLINE       inline __attribute__((always_inline))
#endif


// Alternative to [[maybe_unused]] which can be applied to a statement
#define XCI_UNUSED          (void)

#endif // include guard
