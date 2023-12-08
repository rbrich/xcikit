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
    union {
        struct { Vec4<T> c1, c2, c3, c4; };  // columns
        std::array<T, 4*4> arr;
    };

    constexpr Mat4() = default;
    constexpr Mat4(Vec4<T> c1, Vec4<T> c2, Vec4<T> c3, Vec4<T> c4) : c1(c1), c2(c2), c3(c3), c4(c4) {}
    constexpr Mat4(T x1, T y1, T z1, T w1,
                   T x2, T y2, T z2, T w2,
                   T x3, T y3, T z3, T w3,
                   T x4, T y4, T z4, T w4)
            : c1{x1, y1, z1, w1}, c2{x2, y2, z2, w2}, c3{x3, y3, z3, w3}, c4{x4, y4, z4, w4} {}

    static constexpr Mat4 identity() {
        return {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1,
        };
    }

    static constexpr Mat4 translate(Vec3<T> t) {
        return {
            1,   0,   0,   0,
            0,   1,   0,   0,
            0,   0,   1,   0,
            t.x, t.y, t.z, 1,
        };
    }

    static constexpr Mat4 scale(Vec3<T> s) {
        return {
            s.x,   0,   0,   0,
            0,   s.y,   0,   0,
            0,   0,   s.z,   0,
            0,   0,     0,   1,
        };
    }

    /// Create a matrix for rotation around X axis, followed by translation by `t`
    static constexpr Mat4 rot_x(T cos, T sin, Vec3<T> t = {}) {
        return {
            1,   0,   0,   0,
            0,   cos, sin, 0,
            0,  -sin, cos, 0,
            t.x, t.y, t.z, 1,
        };
    }

    /// Create a matrix for rotation around Y axis, followed by translation by `t`
    static constexpr Mat4 rot_y(T cos, T sin, Vec3<T> t = {}) {
        return {
            cos, 0,  -sin, 0,
            0,   1,   0,   0,
            sin, 0,   cos, 0,
            t.x, t.y, t.z, 1,
        };
    }

    /// Create a matrix for rotation around Z axis, followed by translation by `t`
    static constexpr Mat4 rot_z(T cos, T sin, Vec3<T> t = {}) {
        return {
            cos, sin, 0,   0,
           -sin, cos, 0,   0,
            0,   0,   1,   0,
            t.x, t.y, t.z, 1,
        };
    }

    constexpr Mat4 transpose() const {
        return {row(0), row(1), row(2), row(3)};
    }

    constexpr T determinant() const {
        return c1.x * mat3({1,2,3}, {1,2,3}).determinant()
             - c2.x * mat3({0,2,3}, {1,2,3}).determinant()
             + c3.x * mat3({0,1,3}, {1,2,3}).determinant()
             - c4.x * mat3({0,1,2}, {1,2,3}).determinant();
    }

    constexpr Mat4 cofactor() const {
        return {
            mat3({1,2,3}, {1,2,3}).determinant(),
           -mat3({1,2,3}, {0,2,3}).determinant(),
            mat3({1,2,3}, {0,1,3}).determinant(),
           -mat3({1,2,3}, {0,1,2}).determinant(),

           -mat3({0,2,3}, {1,2,3}).determinant(),
            mat3({0,2,3}, {0,2,3}).determinant(),
           -mat3({0,2,3}, {0,1,3}).determinant(),
            mat3({0,2,3}, {0,1,2}).determinant(),

            mat3({0,1,3}, {1,2,3}).determinant(),
           -mat3({0,1,3}, {0,2,3}).determinant(),
            mat3({0,1,3}, {0,1,3}).determinant(),
           -mat3({0,1,3}, {0,1,2}).determinant(),

           -mat3({0,1,2}, {1,2,3}).determinant(),
            mat3({0,1,2}, {0,2,3}).determinant(),
           -mat3({0,1,2}, {0,1,3}).determinant(),
            mat3({0,1,2}, {0,1,2}).determinant(),
        };
    }

    constexpr Mat4 inverse_transpose() const {
        const auto cof = cofactor();
        const auto det = c1.x * cof.c1.x + c2.x * cof.c2.x + c3.x * cof.c3.x + c4.x * cof.c4.x;
        assert(det == determinant());
        return cof * static_cast<T>(1.0 / det);
    }

    constexpr Mat4 inverse() const {
        return inverse_transpose().transpose();
    }

    constexpr const Vec4<T>& col(unsigned i) const {
        switch (i) {
            case 0: return c1;
            case 1: return c2;
            case 2: return c3;
            case 3: return c4;
        }
        XCI_UNREACHABLE;
    }

    constexpr Vec4<T> row(unsigned i) const {
        return {c1[i], c2[i], c3[i], c4[i]};
    }

    constexpr Mat3<T> mat3(Vec3u cols, Vec3u rows) const {
        return {
            col(cols.x).vec3(rows),
            col(cols.y).vec3(rows),
            col(cols.z).vec3(rows),
        };
    }

    constexpr Mat3<T> mat3() const {
        return {c1.vec3(), c2.vec3(), c3.vec3()};
    }

    constexpr Mat4<T>& operator *=(const Mat4<T>& rhs) {
        return *this = *this * rhs;;
    }

    constexpr const Vec4<T>& operator[] (unsigned i) const { return col(i); }

    constexpr explicit operator bool() const noexcept {
        return c1 || c2 || c3 || c4;
    }

    constexpr const T* data() const { return arr.data(); }
    constexpr size_t byte_size() const { return sizeof(T) * arr.size(); }
};


// Mat4 * T
template <typename T>
constexpr Mat4<T> operator *(const Mat4<T>& lhs, T rhs) {
    return Mat4<T>(lhs.c1 * rhs, lhs.c2 * rhs, lhs.c3 * rhs, lhs.c4 * rhs);
}

// Mat4 * Vec4
template <typename T>
constexpr Vec4<T> operator *(const Mat4<T>& lhs, const Vec4<T>& rhs) {
    return {lhs.row(0).dot(rhs),
            lhs.row(1).dot(rhs),
            lhs.row(2).dot(rhs),
            lhs.row(3).dot(rhs)};
}

// Mat4 * Mat4
template <typename T>
constexpr Mat4<T> operator *(const Mat4<T>& lhs, const Mat4<T>& rhs) {
    return {lhs * rhs.c1,
            lhs * rhs.c2,
            lhs * rhs.c3,
            lhs * rhs.c4};
}

template <typename T>
constexpr bool operator ==(const Mat4<T>& lhs, const Mat4<T>& rhs) {
    return lhs.c1 == rhs.c1 && lhs.c2 == rhs.c2 && lhs.c3 == rhs.c3 && lhs.c4 == rhs.c4;
}

template <typename T>
std::ostream& operator <<(std::ostream& s, Mat4<T> m) {
    return s << "{{" << m.c1.x << ", " << m.c1.y << ", " << m.c1.z << ", " << m.c1.w << "},"
             << " {" << m.c2.x << ", " << m.c2.y << ", " << m.c2.z << ", " << m.c2.w << "},"
             << " {" << m.c3.x << ", " << m.c3.y << ", " << m.c3.z << ", " << m.c3.w << "},"
             << " {" << m.c4.x << ", " << m.c4.y << ", " << m.c4.z << ", " << m.c4.w << "}}";
}


using Mat4f = Mat4<float>;


} // namespace xci

#endif // include guard
