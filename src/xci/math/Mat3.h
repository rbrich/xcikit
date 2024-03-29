// Mat3.h created on 2023-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_MATH_MAT3_H
#define XCI_MATH_MAT3_H

#include "Vec3.h"
#include "Mat2.h"

namespace xci {


// Column-major 3x3 matrix, same as in GLSL and glm
template <typename T>
struct Mat3 {
    union {
        struct { Vec3<T> c1, c2, c3; };  // columns
        std::array<T, 3*3> arr;
    };

    constexpr Mat3() = default;
    constexpr Mat3(Vec3<T> c1, Vec3<T> c2, Vec3<T> c3) : c1(c1), c2(c2), c3(c3) {}
    constexpr Mat3(T x1, T y1, T z1,
                   T x2, T y2, T z2,
                   T x3, T y3, T z3) : c1{x1, y1, z1}, c2{x2, y2, z2}, c3{x3, y3, z3} {}

    static constexpr Mat3 identity() {
        return {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
        };
    }

    constexpr Mat3 transpose() const {
        return {row(0), row(1), row(2)};
    }

    constexpr Mat3 inverse() const {
        return Mat3{
             (c2.y * c3.z - c3.y * c2.z),
            -(c1.y * c3.z - c3.y * c1.z),
             (c1.y * c2.z - c2.y * c1.z),
            -(c2.x * c3.z - c3.x * c2.z),
             (c1.x * c3.z - c3.x * c1.z),
            -(c1.x * c2.z - c2.x * c1.z),
             (c2.x * c3.y - c3.x * c2.y),
            -(c1.x * c3.y - c3.x * c1.y),
             (c1.x * c2.y - c2.x * c1.y),
        } * (static_cast<T>(1.0) / determinant());
    }

    constexpr T determinant() const {
        // Leibniz formula
        return
            c1.x * (c2.y * c3.z - c3.y * c2.z)
          - c2.x * (c1.y * c3.z - c3.y * c1.z)
          + c3.x * (c1.y * c2.z - c2.y * c1.z);
    }

    constexpr Mat2<T> mat2(unsigned col1, unsigned col2, unsigned row1, unsigned row2) const {
        return {
            col(col1).vec2(row1, row2),
            col(col2).vec2(row1, row2)
        };
    }

    constexpr const Vec3<T>& col(unsigned i) const {
        switch (i) {
            case 0: return c1;
            case 1: return c2;
            case 2: return c3;
        }
        XCI_UNREACHABLE;
    }

    constexpr Vec3<T> row(unsigned i) const {
        return {c1[i], c2[i], c3[i]};
    }

    constexpr const Vec3<T>& operator[] (unsigned i) const { return col(i); }

    constexpr explicit operator bool() const noexcept {
        return c1 || c2 || c3;
    }

    constexpr const T* data() const { return arr.data(); }
    constexpr size_t byte_size() const { return sizeof(T) * arr.size(); }
};


// Mat3 * T
template <typename T>
constexpr Mat3<T> operator *(const Mat3<T>& lhs, T rhs) {
    return Mat3<T>(lhs.c1 * rhs, lhs.c2 * rhs, lhs.c3 * rhs);
}

// Mat3 * Vec3
template <typename T>
constexpr Vec3<T> operator *(const Mat3<T>& lhs, const Vec3<T>& rhs) {
    return {lhs.row(0).dot(rhs),
            lhs.row(1).dot(rhs),
            lhs.row(2).dot(rhs)};
}

// Mat3 * Mat3
template <typename T>
constexpr Mat3<T> operator *(const Mat3<T>& lhs, const Mat3<T>& rhs) {
    return {lhs * rhs.c1,
            lhs * rhs.c2,
            lhs * rhs.c3};
}

template <typename T>
constexpr bool operator ==(const Mat3<T>& lhs, const Mat3<T>& rhs) {
    return lhs.c1 == rhs.c1 && lhs.c2 == rhs.c2 && lhs.c3 == rhs.c3;
}

template <typename T>
std::ostream& operator <<(std::ostream& s, Mat3<T> m) {
    return s << "{{" << m.c1.x << ", " << m.c1.y << ", " << m.c1.z << "},"
             << " {" << m.c2.x << ", " << m.c2.y << ", " << m.c2.z << "},"
             << " {" << m.c3.x << ", " << m.c3.y << ", " << m.c3.z << "}}";
}


using Mat3f = Mat3<float>;


} // namespace xci

#endif // include guard
