// Vec2.h created on 2018-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_MATH_VEC2_H
#define XCI_MATH_VEC2_H

#include <xci/compat/macros.h>
#include <cmath>
#include <algorithm>
#include <ostream>
#include <cstdint>

namespace xci {


// Return value as POD numeric type.
template<typename T, typename std::enable_if_t<std::is_arithmetic<T>::value, int> = 0>
T cast_to_numeric(T var) { return var; }

// Specialization for graphics::Units and other types defining 'numeric_type' as member.
template<typename T>
typename T::numeric_type cast_to_numeric(T var)
{ return static_cast<typename T::numeric_type>(var); }


template <typename T>
struct Vec2 {
    T x {};
    T y {};

    constexpr Vec2() = default;
    constexpr Vec2(T x, T y) : x(x), y(y) {}

    // Convert another type of vector (possibly foreign type)
    template <typename TVec>
    constexpr explicit Vec2(const TVec& other) : x(other.x), y(other.y) {}

    constexpr T length() const {
        return static_cast<T>(std::hypot(
                cast_to_numeric(x),
                cast_to_numeric(y)));
    }

    constexpr Vec2<T> norm() const {
        auto l = length();
        return { x / l, y / l };
    }

    constexpr T dot(const Vec2<T>& rhs) const {
        return x * rhs.x + y * rhs.y;
    }

    constexpr T dist(const Vec2<T>& other) const {
        return static_cast<T>(std::hypot(
                cast_to_numeric(x - other.x),
                cast_to_numeric(y - other.y)));
    }

    constexpr T dist_squared(const Vec2<T>& other) const {
        auto dx = x - other.x;
        auto dy = y - other.y;
        return dx*dx + dy*dy;
    }

    constexpr T dist_taxicab(const Vec2<T>& other) const {
        auto dx = cast_to_numeric(x - other.x);
        auto dy = cast_to_numeric(y - other.y);
        return std::abs(dx) + std::abs(dy);
    }

    constexpr Vec2<T> rotate(float angle_radians) const {
        const float c = std::cos(angle_radians);
        const float s = std::sin(angle_radians);
        return {x * c - y * s, x * s + y * c};
    }

    constexpr const T& at(unsigned i) const { return operator[](i); }

    constexpr Vec2<T>& operator +=(const Vec2<T>& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    constexpr Vec2<T>& operator -=(const Vec2<T>& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    constexpr T& operator[] (unsigned i) {
        switch (i) {
            case 0: return x;
            case 1: return y;
        }
        XCI_UNREACHABLE;
    }

    constexpr const T& operator[] (unsigned i) const {
        switch (i) {
            case 0: return x;
            case 1: return y;
        }
        XCI_UNREACHABLE;
    }

    constexpr explicit operator bool() const noexcept {
        return x != T{} || y != T{};
    }
};

// unary minus (opposite vector)
template <typename T>
constexpr Vec2<T> operator -(const Vec2<T>& rhs) {
    return Vec2<T>(-rhs.x, -rhs.y);
}

template <typename T>
constexpr Vec2<T> operator +(const Vec2<T>& lhs, const Vec2<T>& rhs) {
    return Vec2<T>(lhs.x + rhs.x, lhs.y + rhs.y);
}

template <typename T>
constexpr Vec2<T> operator +(const Vec2<T>& lhs, T rhs) {
    return Vec2<T>(lhs.x + rhs, lhs.y + rhs);
}

template <typename T>
constexpr Vec2<T> operator -(const Vec2<T>& lhs, const Vec2<T>& rhs) {
    return Vec2<T>(lhs.x - rhs.x, lhs.y - rhs.y);
}

template <typename T>
constexpr Vec2<T> operator *(const Vec2<T>& lhs, const Vec2<T>& rhs) {
    return Vec2<T>(lhs.x * rhs.x, lhs.y * rhs.y);
}

template <typename T>
constexpr Vec2<T> operator *(const Vec2<T>& lhs, T rhs) {
    return Vec2<T>(lhs.x * rhs, lhs.y * rhs);
}

template <typename T, typename U>
constexpr Vec2<T> operator *(U lhs, const Vec2<T>& rhs) {
    return Vec2<T>(lhs * rhs.x, lhs * rhs.y);
}

template <typename T>
constexpr Vec2<T> operator /(const Vec2<T>& lhs, const Vec2<T>& rhs) {
    return Vec2<T>(lhs.x / rhs.x, lhs.y / rhs.y);
}

template <typename T>
constexpr Vec2<T> operator /(const Vec2<T>& lhs, T rhs) {
    return Vec2<T>(lhs.x / rhs, lhs.y / rhs);
}

template <typename T>
constexpr bool operator ==(const Vec2<T>& lhs, const Vec2<T>& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
std::ostream& operator <<(std::ostream& s, Vec2<T> v) {
    return s << "{" << v.x << ", " << v.y << "}";
}

using Vec2i = Vec2<int32_t>;
using Vec2u = Vec2<uint32_t>;
using Vec2f = Vec2<float>;


} // namespace xci

#endif // include guard
