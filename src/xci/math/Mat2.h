// Mat2.h created on 2023-11-24 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_MATH_MAT2_H
#define XCI_MATH_MAT2_H

#include "Vec2.h"

namespace xci {


// Column-major 2x2 matrix, same as in GLSL and glm
template <typename T>
struct Mat2 {
    union {
        struct { Vec2<T> c1, c2; };  // columns
        std::array<T, 2*2> arr;
    };

    constexpr Mat2() = default;
    constexpr Mat2(Vec2<T> c1, Vec2<T> c2) : c1(c1), c2(c2) {}
    constexpr Mat2(T x1, T y1,
         T x2, T y2) : c1{x1, y1}, c2{x2, y2} {}

    static constexpr Mat2 identity() {
        return {
            1, 0,
            0, 1,
        };
    }

    constexpr T determinant() const {
        return c1.x * c2.y - c2.x * c1.y;
    }

    constexpr const Vec2<T>& col(unsigned i) const {
        switch (i) {
            case 0: return c1;
            case 1: return c2;
        }
        XCI_UNREACHABLE;
    }

    constexpr Vec2<T> row(unsigned i) const {
        return {c1[i], c2[i]};
    }

    constexpr T& operator[] (unsigned i) const { return col(i); }

    constexpr explicit operator bool() const noexcept {
        return c1 || c2;
    }

    constexpr const T* data() const { return arr.data(); }
    constexpr size_t byte_size() const { return sizeof(T) * arr.size(); }
};


using Mat2f = Mat2<float>;


} // namespace xci

#endif // include guard
