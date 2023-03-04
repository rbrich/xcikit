// Mat3.h created on 2023-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_GEOMETRY_MAT3_H
#define XCI_CORE_GEOMETRY_MAT3_H

#include "Vec3.h"

namespace xci::core {


// Column-major 3x3 matrix, same as in GLSL and glm
template <typename T>
struct Mat3 {
    Mat3() = default;
    Mat3(Vec3<T> c1, Vec3<T> c2, Vec3<T> c3) : c1(c1), c2(c2), c3(c3) {}
    Mat3(T x1, T y1, T z1,
         T x2, T y2, T z2,
         T x3, T y3, T z3) : c1{x1, y1, z1}, c2{x2, y2, z2}, c3{x3, y3, z3} {}

    static Mat3 identity() {
        return {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
        };
    }

    T determinant() const {
        // Leibniz formula
        return
            c1.x * (c2.y * c3.z - c3.y * c2.z)
          - c2.x * (c1.y * c3.z - c3.y * c1.z)
          + c3.x * (c1.y * c2.z - c2.y * c1.z);
    }

    Mat3<T>& operator +=(const Mat3<T>& rhs) {
        c1 += rhs.c1;
        c2 += rhs.c2;
        c3 += rhs.c3;
        return *this;
    }

    Mat3<T>& operator -=(const Mat3<T>& rhs) {
        c1 -= rhs.c1;
        c2 -= rhs.c2;
        c3 -= rhs.c3;
        return *this;
    }


    T& operator[] (unsigned i) const {
        switch (i) {
            case 0: return c1;
            case 1: return c2;
            case 2: return c3;
        }
        XCI_UNREACHABLE;
    }

    explicit operator bool() const noexcept {
        return c1 || c2 || c3;
    }

    // columns
    Vec3<T> c1 {};
    Vec3<T> c2 {};
    Vec3<T> c3 {};
};


using Mat3f = Mat3<float>;


} // namespace xci::core

#endif // include guard
