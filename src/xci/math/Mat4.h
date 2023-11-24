// Mat4.h created on 2023-11-24 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_MATH_MAT4_H
#define XCI_MATH_MAT4_H

#include "Vec4.h"
#include "Mat3.h"

#include <cassert>

namespace xci {


// Column-major 4x4 matrix, same as in GLSL and glm
template <typename T>
struct Mat4 {
    // columns
    Vec4<T> c1 {};
    Vec4<T> c2 {};
    Vec4<T> c3 {};
    Vec4<T> c4 {};

    Mat4() = default;
    Mat4(Vec4<T> c1, Vec4<T> c2, Vec4<T> c3, Vec4<T> c4) : c1(c1), c2(c2), c3(c3), c4(c4) {}
    Mat4(T x1, T y1, T z1, T w1,
         T x2, T y2, T z2, T w2,
         T x3, T y3, T z3, T w3,
         T x4, T y4, T z4, T w4)
            : c1{x1, y1, z1, w1}, c2{x2, y2, z2, w2}, c3{x3, y3, z3, w3}, c4{x4, y4, z4, w4} {}

    static Mat4 identity() {
        return {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1,
        };
    }

    static Mat4 translation(Vec3<T> t) {
        return {
            Vec4<T>(1,   0,   0,   0),
            Vec4<T>(0,   1,   0,   0),
            Vec4<T>(0,   0,   1,   0),
            Vec4<T>(t.x, t.y, t.z, 1),
        };
    }

    /// Create a matrix for rotation around X axis, followed by translation by `t`
    static Mat4 rot_x(T cos, T sin, Vec3<T> t = {}) {
        return {
            Vec4<T>(1,   0,   0,   0),
            Vec4<T>(0,   cos, sin, 0),
            Vec4<T>(0,  -sin, cos, 0),
            Vec4<T>(t.x, t.y, t.z, 1),
        };
    }

    /// Create a matrix for rotation around Y axis, followed by translation by `t`
    static Mat4 rot_y(T cos, T sin, Vec3<T> t = {}) {
        return {
            Vec4<T>(cos, 0,  -sin, 0),
            Vec4<T>(0,   1,   0,   0),
            Vec4<T>(sin, 0,   cos, 0),
            Vec4<T>(t.x, t.y, t.z, 1),
        };
    }

    /// Create a matrix for rotation around Z axis, followed by translation by `t`
    static Mat4 rot_z(T cos, T sin, Vec3<T> t = {}) {
        return {
            Vec4<T>( cos, sin, 0,   0),
            Vec4<T>(-sin, cos, 0,   0),
            Vec4<T>( 0,   0,   1,   0),
            Vec4<T>( t.x, t.y, t.z, 1),
        };
    }

    Mat4 transpose() const {
        return {row(0), row(1), row(2), row(3)};
    }

    T determinant() const {
        return c1.x * mat3({1,2,3}, {1,2,3}).determinant()
             - c2.x * mat3({0,2,3}, {1,2,3}).determinant()
             + c3.x * mat3({0,1,3}, {1,2,3}).determinant()
             - c4.x * mat3({0,1,2}, {1,2,3}).determinant();
    }

    Mat4 cofactor() const {
        return {
            Vec4<T>{
                mat3({1,2,3}, {1,2,3}).determinant(),
               -mat3({1,2,3}, {0,2,3}).determinant(),
                mat3({1,2,3}, {0,1,3}).determinant(),
               -mat3({1,2,3}, {0,1,2}).determinant()},
            Vec4<T>{
               -mat3({0,2,3}, {1,2,3}).determinant(),
                mat3({0,2,3}, {0,2,3}).determinant(),
               -mat3({0,2,3}, {0,1,3}).determinant(),
                mat3({0,2,3}, {0,1,2}).determinant()},
            Vec4<T>{
                mat3({0,1,3}, {1,2,3}).determinant(),
               -mat3({0,1,3}, {0,2,3}).determinant(),
                mat3({0,1,3}, {0,1,3}).determinant(),
               -mat3({0,1,3}, {0,1,2}).determinant()},
            Vec4<T>{
               -mat3({0,1,2}, {1,2,3}).determinant(),
                mat3({0,1,2}, {0,2,3}).determinant(),
               -mat3({0,1,2}, {0,1,3}).determinant(),
                mat3({0,1,2}, {0,1,2}).determinant()}
        };
    }

    Mat4 inverse_transpose() const {
        const auto cof = cofactor();
        const auto det = c1.x * cof.c1.x + c2.x * cof.c2.x + c3.x * cof.c3.x + c4.x * cof.c4.x;
        assert(det == determinant());
        return cof * static_cast<T>(1.0 / det);
    }

    Mat4 inverse() const {
        return inverse_transpose().transpose();
    }

    const Vec4<T>& col(unsigned i) const {
        switch (i) {
            case 0: return c1;
            case 1: return c2;
            case 2: return c3;
            case 3: return c4;
        }
        XCI_UNREACHABLE;
    }

    Vec4<T> row(unsigned i) const {
        return {c1[i], c2[i], c3[i], c4[i]};
    }

    Mat3<T> mat3(Vec3u cols, Vec3u rows) const {
        return {
            col(cols.x).vec3(rows),
            col(cols.y).vec3(rows),
            col(cols.z).vec3(rows),
        };
    }

    Mat3<T> mat3() const {
        return {c1.vec3(), c2.vec3(), c3.vec3()};
    }

    Mat4<T>& operator *=(const Mat4<T>& rhs) {
        return *this = *this * rhs;;
    }

    const Vec4<T>& operator[] (unsigned i) const { return col(i); }

    explicit operator bool() const noexcept {
        return c1 || c2 || c3 || c4;
    }

    bool operator==(const Mat4& rhs) const { return c1 == rhs.c1 && c2 == rhs.c2 && c3 == rhs.c3 && c4 == rhs.c4; }
};


// Mat4 * T
template <typename T>
Mat4<T> operator *(const Mat4<T>& lhs, T rhs) {
    return Mat4<T>(lhs.c1 * rhs, lhs.c2 * rhs, lhs.c3 * rhs, lhs.c4 * rhs);
}

// Mat4 * Vec4
template <typename T>
Vec4<T> operator *(const Mat4<T>& lhs, const Vec4<T>& rhs) {
    return {lhs.row(0).dot(rhs),
            lhs.row(1).dot(rhs),
            lhs.row(2).dot(rhs),
            lhs.row(3).dot(rhs)};
}

// Mat4 * Mat4
template <typename T>
Mat4<T> operator *(const Mat4<T>& lhs, const Mat4<T>& rhs) {
    return {lhs * rhs.c1,
            lhs * rhs.c2,
            lhs * rhs.c3,
            lhs * rhs.c4};
}


using Mat4f = Mat4<float>;


} // namespace xci

#endif // include guard
