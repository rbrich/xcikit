// float128.h created on 2023-09-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

// References:
// - https://en.cppreference.com/w/cpp/types/floating-point (C++23 <stdfloat>)
// - https://gcc.gnu.org/onlinedocs/gcc/Floating-Types.html (__float128)
// - https://en.wikipedia.org/wiki/Quadruple-precision_floating-point_format

#ifndef XCI_COMPAT_FLOAT128_H
#define XCI_COMPAT_FLOAT128_H

#if defined(HAVE_GNU_EXT_FLOAT128)

using float128 = __float128;

#else

// Fake it, but make sure it's 128bits wide
struct float128 {
    using type = long double;
    union {
        type value;
        char __storage[16];
    };

    float128(double v) : value(v) {}
    operator double() const noexcept { return value; }
};

template<> class std::numeric_limits<float128> {
public:
    using base = std::numeric_limits<float128::type>;
    static constexpr int digits = base::digits;
    static constexpr int digits10 = base::digits10;
    static constexpr int max_digits10 = base::max_digits10;
};

#endif

#endif // include guard
