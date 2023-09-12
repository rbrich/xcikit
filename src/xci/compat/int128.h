// int128.h created on 2023-09-06 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

// References:
// - https://quuxplusone.github.io/blog/2019/02/28/is-int128-integral/

#ifndef XCI_COMPAT_INT128_H
#define XCI_COMPAT_INT128_H

#include <string>
#include <sstream>
#include <iomanip>

#if defined(__GNUC__)

using uint128 = __uint128_t;
using int128 = __int128_t;

#elif defined(_MSC_VER)

#include <__msvc_int128.hpp>
using uint128 = std::_Unsigned128;
using int128 = std::_Signed128;

#else

#error "Unsupported compiler"

#endif

// Inspired by Abseil
// https://github.com/abseil/abseil-cpp/blob/master/absl/numeric/int128.cc

inline std::string uint128_to_string(uint128 v) {
    constexpr uint64_t divider = 10000000000000000000u;
    constexpr uint64_t zeros = 19;
    auto part3 = uint64_t(v % divider);
    v /= divider;
    auto part2 = uint64_t(v % divider);
    v /= divider;
    std::ostringstream os;
    if (v != 0) {
        os << uint64_t(v)
           << std::noshowbase << std::setfill('0')
           << std::setw(zeros) << part2
           << std::setw(zeros) << part3;
    } else if (part2 != 0) {
        os << part2
           << std::noshowbase << std::setfill('0')
           << std::setw(zeros) << part3;
    } else {
        os << part3;
    }
    return os.str();
}

inline std::string int128_to_string(int128 v) {
    std::string res;
    const bool negative = v < 0;
    if (negative)
        res += '-';
    res += uint128_to_string(negative? -uint128(v) : uint128(v));
    return res;
}

#endif // include guard
