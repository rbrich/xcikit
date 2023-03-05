// Vec3.h created on 2023-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GEOMETRY_VEC3_H
#define XCI_GEOMETRY_VEC3_H

#include <xci/compat/macros.h>
#include <ostream>

namespace xci::core {


template <typename T>
struct Vec3 {
    Vec3() = default;
    Vec3(T x, T y, T z) : x(x), y(y), z(z) {}

    // Convert another type of vector (possibly foreign type)
    template <typename TVec>
    explicit Vec3(const TVec& other) : x(other.x), y(other.y), z(other.z) {}

    Vec3<T>& operator +=(const Vec3<T>& rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    Vec3<T>& operator -=(const Vec3<T>& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    T& operator[] (unsigned i) const {
        switch (i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
        }
        XCI_UNREACHABLE;
    }

    explicit operator bool() const noexcept {
        return x != T{} || y != T{} || z != T{};
    }

public:
    T x {};
    T y {};
    T z {};
};

// unary minus (opposite vector)
template <typename T>
Vec3<T> operator -(const Vec3<T>& rhs) {
    return Vec3<T>(-rhs.x, -rhs.y, -rhs.z);
}

template <typename T>
Vec3<T> operator +(const Vec3<T>& lhs, const Vec3<T>& rhs) {
    return Vec3<T>(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
}

template <typename T>
Vec3<T> operator +(const Vec3<T>& lhs, T rhs) {
    return Vec3<T>(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs);
}

template <typename T>
Vec3<T> operator -(const Vec3<T>& lhs, const Vec3<T>& rhs) {
    return Vec3<T>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}

template <typename T>
Vec3<T> operator *(const Vec3<T>& lhs, const Vec3<T>& rhs) {
    return Vec3<T>(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
}

template <typename T>
Vec3<T> operator *(const Vec3<T>& lhs, T rhs) {
    return Vec3<T>(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
}

template <typename T, typename U>
Vec3<T> operator *(U lhs, const Vec3<T>& rhs) {
    return Vec3<T>(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z);
}

template <typename T>
Vec3<T> operator /(const Vec3<T>& lhs, const Vec3<T>& rhs) {
    return Vec3<T>(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z);
}

template <typename T>
Vec3<T> operator /(const Vec3<T>& lhs, T rhs) {
    return Vec3<T>(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
}

template <typename T>
bool operator ==(const Vec3<T>& lhs, const Vec3<T>& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

template <typename T>
bool operator !=(const Vec3<T>& lhs, const Vec3<T>& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
}

template <typename T>
std::ostream& operator <<(std::ostream& s, Vec3<T> v) {
    return s << "{" << v.x << ", " << v.y << ", " << v.z << "}";
}

using Vec3f = Vec3<float>;


} // namespace xci::core

#endif // include guard
