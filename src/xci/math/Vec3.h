// Vec3.h created on 2023-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_MATH_VEC3_H
#define XCI_MATH_VEC3_H

#include "Vec2.h"
#include <xci/compat/macros.h>
#include <ostream>

namespace xci {


template <typename T>
struct Vec3 {
    union {
        struct { T x, y, z; };
        std::array<T, 3> arr;
    };

    constexpr Vec3() : arr({}) {}
    constexpr Vec3(T x, T y, T z) : x(x), y(y), z(z) {}

    // Convert another type of vector (possibly foreign type)
    template <typename TVec>
    constexpr explicit Vec3(const TVec& other) : x(other.x), y(other.y), z(other.z) {}

    constexpr T length() const {
        return static_cast<T>(std::sqrt(cast_to_numeric(x * x + y * y + z * z)));
    }

    constexpr Vec3 normalize() const {
        const T il = 1.0 / length();
        return {x * il, y * il, z * il};
    }

    constexpr Vec3 cross(const Vec3& rhs) const {
        return Vec3(y * rhs.z - z * rhs.y,
                    z * rhs.x - x * rhs.z,
                    x * rhs.y - y * rhs.x);
    }

    constexpr T dot(const Vec3& rhs) const {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    constexpr Vec2<T> vec2(unsigned i1, unsigned i2) const {
        return Vec2(at(i1), at(i2));
    }

    constexpr const T& at(unsigned i) const { return operator[](i); }

    constexpr Vec3& operator +=(const Vec3& rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    constexpr Vec3& operator -=(const Vec3& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    constexpr T& operator[] (unsigned i) { return arr[i]; }
    constexpr const T& operator[] (unsigned i) const { return arr[i]; }

    constexpr explicit operator bool() const noexcept {
        return x != T{} || y != T{} || z != T{};
    }

    constexpr const T* data() const { return arr.data(); }
    constexpr size_t byte_size() const { return sizeof(T) * arr.size(); }
};

// unary minus (opposite vector)
template <typename T>
constexpr Vec3<T> operator -(const Vec3<T>& rhs) {
    return Vec3<T>(-rhs.x, -rhs.y, -rhs.z);
}

template <typename T>
constexpr Vec3<T> operator +(const Vec3<T>& lhs, const Vec3<T>& rhs) {
    return Vec3<T>(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
}

template <typename T>
constexpr Vec3<T> operator +(const Vec3<T>& lhs, T rhs) {
    return Vec3<T>(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs);
}

template <typename T>
constexpr Vec3<T> operator -(const Vec3<T>& lhs, const Vec3<T>& rhs) {
    return Vec3<T>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}

template <typename T>
constexpr Vec3<T> operator *(const Vec3<T>& lhs, const Vec3<T>& rhs) {
    return Vec3<T>(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
}

template <typename T>
constexpr Vec3<T> operator *(const Vec3<T>& lhs, T rhs) {
    return Vec3<T>(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
}

template <typename T, typename U>
constexpr Vec3<T> operator *(U lhs, const Vec3<T>& rhs) {
    return Vec3<T>(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z);
}

template <typename T>
constexpr Vec3<T> operator /(const Vec3<T>& lhs, const Vec3<T>& rhs) {
    return Vec3<T>(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z);
}

template <typename T>
constexpr Vec3<T> operator /(const Vec3<T>& lhs, T rhs) {
    return Vec3<T>(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
}

template <typename T>
constexpr bool operator ==(const Vec3<T>& lhs, const Vec3<T>& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

template <typename T>
std::ostream& operator <<(std::ostream& s, Vec3<T> v) {
    return s << "{" << v.x << ", " << v.y << ", " << v.z << "}";
}

using Vec3i = Vec3<int32_t>;
using Vec3u = Vec3<uint32_t>;
using Vec3f = Vec3<float>;


} // namespace xci

#endif // include guard
