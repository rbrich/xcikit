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
    // columns
    Vec2<T> c1 {};
    Vec2<T> c2 {};

    Mat2() = default;
    Mat2(Vec2<T> c1, Vec2<T> c2) : c1(c1), c2(c2) {}
    Mat2(T x1, T y1,
         T x2, T y2) : c1{x1, y1}, c2{x2, y2} {}

    static Mat2 identity() {
        return {
            1, 0,
            0, 1,
        };
    }

    T determinant() const {
        return c1.x * c2.y - c2.x * c1.y;
    }

    const Vec2<T>& col(unsigned i) const {
        switch (i) {
            case 0: return c1;
            case 1: return c2;
        }
        XCI_UNREACHABLE;
    }

    Vec2<T> row(unsigned i) const {
        return {c1[i], c2[i]};
    }

    T& operator[] (unsigned i) const { return col(i); }

    explicit operator bool() const noexcept {
        return c1 || c2;
    }
};


using Mat2f = Mat2<float>;


} // namespace xci

#endif // include guard
