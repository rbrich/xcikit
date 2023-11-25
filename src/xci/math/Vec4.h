// Vec4.h created on 2023-11-24 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_MATH_VEC4_H
#define XCI_MATH_VEC4_H

#include "Vec3.h"
#include <xci/compat/macros.h>
#include <ostream>

namespace xci {


template <typename T>
struct Vec4 {
    T x {};
    T y {};
    T z {};
    T w {};

    constexpr Vec4() = default;
    constexpr Vec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

    // Convert another type of vector (possibly foreign type)
    template <typename TVec>
    constexpr explicit Vec4(const TVec& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}

    constexpr T dot(const Vec4<T>& rhs) const {
        return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
    }

    constexpr Vec3<T> vec3(Vec3u i) const {
        return {at(i.x), at(i.y), at(i.z)};
    }

    constexpr Vec3<T> vec3() const { return {x, y, z}; }

    constexpr const T& at(unsigned i) const { return operator[](i); }

    constexpr Vec4<T>& operator +=(const Vec4<T>& rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    constexpr Vec4<T>& operator -=(const Vec4<T>& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    constexpr T& operator[] (unsigned i) {
        switch (i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            case 3: return w;
        }
        XCI_UNREACHABLE;
    }

    constexpr const T& operator[] (unsigned i) const {
        switch (i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            case 3: return w;
        }
        XCI_UNREACHABLE;
    }

    constexpr explicit operator bool() const noexcept {
        return x != T{} || y != T{} || z != T{} || w != T{};
    }
};

// unary minus (opposite vector)
template <typename T>
constexpr Vec4<T> operator -(const Vec4<T>& rhs) {
    return Vec4<T>(-rhs.x, -rhs.y, -rhs.z, -rhs.w);
}

template <typename T>
constexpr Vec4<T> operator +(const Vec4<T>& lhs, const Vec4<T>& rhs) {
    return Vec4<T>(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
}

template <typename T>
constexpr Vec4<T> operator +(const Vec4<T>& lhs, T rhs) {
    return Vec4<T>(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs, lhs.w + rhs);
}

template <typename T>
constexpr Vec4<T> operator -(const Vec4<T>& lhs, const Vec4<T>& rhs) {
    return Vec4<T>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
}

template <typename T>
constexpr Vec4<T> operator *(const Vec4<T>& lhs, const Vec4<T>& rhs) {
    return Vec4<T>(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
}

template <typename T>
constexpr Vec4<T> operator *(const Vec4<T>& lhs, T rhs) {
    return Vec4<T>(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs);
}

template <typename T, typename U>
constexpr Vec4<T> operator *(U lhs, const Vec4<T>& rhs) {
    return Vec4<T>(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w);
}

template <typename T>
constexpr Vec4<T> operator /(const Vec4<T>& lhs, const Vec4<T>& rhs) {
    return Vec4<T>(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w);
}

template <typename T>
constexpr Vec4<T> operator /(const Vec4<T>& lhs, T rhs) {
    return Vec4<T>(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs);
}

template <typename T>
constexpr bool operator ==(const Vec4<T>& lhs, const Vec4<T>& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

template <typename T>
std::ostream& operator <<(std::ostream& s, Vec4<T> v) {
    return s << "{" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "}";
}

using Vec4i = Vec4<int32_t>;
using Vec4u = Vec4<uint32_t>;
using Vec4f = Vec4<float>;


} // namespace xci

#endif // include guard
