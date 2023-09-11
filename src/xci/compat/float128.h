// float128.h created on 2023-09-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

// References:
// - https://en.cppreference.com/w/cpp/types/floating-point (C++23 <stdfloat>)

#ifndef XCI_COMPAT_FLOAT128_H
#define XCI_COMPAT_FLOAT128_H

#if defined(__GNUC__)

// Long double might be 80-bit or fake double-double, but it should be 128bits wide.
// Let's wait for <stdfloat> for the proper thing.
static_assert(sizeof(long double) == 16, "Target doesn't support 128bit float");
using float128 = long double;

#elif defined(_MSC_VER)

// Fake it, but make sure it's 128bits wide
struct float128 {
    union {
        long double value;
        char __storage[16];
    };

    float128(double v) : value(v) {}
    operator double() const noexcept { return value; }
};

#else

#error "Unsupported compiler"

#endif

#endif // include guard
