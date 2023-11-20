// Vec2.h created on 2018-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_MATH_VEC2_H
#define XCI_MATH_VEC2_H

#include <cmath>
#include <algorithm>
#include <ostream>
#include <cstdint>

namespace xci::core {


// Return value as POD numeric type.
template<typename T, typename std::enable_if_t<std::is_arithmetic<T>::value, int> = 0>
T cast_to_numeric(T var) { return var; }

// Specialization for graphics::Units and other types defining 'numeric_type' as member.
template<typename T>
typename T::numeric_type cast_to_numeric(T var)
{ return static_cast<typename T::numeric_type>(var); }


template <typename T>
struct Vec2 {
    Vec2() = default;
    Vec2(T x, T y) : x(x), y(y) {}

    // Convert another type of vector (possibly foreign type)
    template <typename TVec>
    explicit Vec2(const TVec& other) : x(other.x), y(other.y) {}

    T length() const {
        return static_cast<T>(std::hypot(
                cast_to_numeric(x),
                cast_to_numeric(y)));
    }

    Vec2<T> norm() const {
        auto l = length();
        return { x / l, y / l };
    }

    T dist(const Vec2<T>& other) const {
        return static_cast<T>(std::hypot(
                cast_to_numeric(x - other.x),
                cast_to_numeric(y - other.y)));
    }

    T dist_squared(const Vec2<T>& other) const {
        auto dx = x - other.x;
        auto dy = y - other.y;
        return dx*dx + dy*dy;
    }

    T dist_taxicab(const Vec2<T>& other) const {
        auto dx = cast_to_numeric(x - other.x);
        auto dy = cast_to_numeric(y - other.y);
        return std::abs(dx) + std::abs(dy);
    }

    Vec2<T> rotate(float angle_radians) const {
        const float c = std::cos(angle_radians);
        const float s = std::sin(angle_radians);
        return {x * c - y * s, x * s + y * c};
    }

    Vec2<T>& operator +=(const Vec2<T>& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    Vec2<T>& operator -=(const Vec2<T>& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    explicit operator bool() const noexcept {
        return x != T{} || y != T{};
    }

public:
    T x {};
    T y {};
};

// unary minus (opposite vector)
template <typename T>
Vec2<T> operator -(const Vec2<T>& rhs) {
    return Vec2<T>(-rhs.x, -rhs.y);
}

template <typename T>
Vec2<T> operator +(const Vec2<T>& lhs, const Vec2<T>& rhs) {
    return Vec2<T>(lhs.x + rhs.x, lhs.y + rhs.y);
}

template <typename T>
Vec2<T> operator +(const Vec2<T>& lhs, T rhs) {
    return Vec2<T>(lhs.x + rhs, lhs.y + rhs);
}

template <typename T>
Vec2<T> operator -(const Vec2<T>& lhs, const Vec2<T>& rhs) {
    return Vec2<T>(lhs.x - rhs.x, lhs.y - rhs.y);
}

template <typename T>
Vec2<T> operator *(const Vec2<T>& lhs, const Vec2<T>& rhs) {
    return Vec2<T>(lhs.x * rhs.x, lhs.y * rhs.y);
}

template <typename T>
Vec2<T> operator *(const Vec2<T>& lhs, T rhs) {
    return Vec2<T>(lhs.x * rhs, lhs.y * rhs);
}

template <typename T, typename U>
Vec2<T> operator *(U lhs, const Vec2<T>& rhs) {
    return Vec2<T>(lhs * rhs.x, lhs * rhs.y);
}

template <typename T>
Vec2<T> operator /(const Vec2<T>& lhs, const Vec2<T>& rhs) {
    return Vec2<T>(lhs.x / rhs.x, lhs.y / rhs.y);
}

template <typename T>
Vec2<T> operator /(const Vec2<T>& lhs, T rhs) {
    return Vec2<T>(lhs.x / rhs, lhs.y / rhs);
}

template <typename T>
bool operator ==(const Vec2<T>& lhs, const Vec2<T>& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
bool operator !=(const Vec2<T>& lhs, const Vec2<T>& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

template <typename T>
std::ostream& operator <<(std::ostream& s, Vec2<T> v) {
    return s << "{" << v.x << ", " << v.y << "}";
}

using Vec2i = Vec2<int32_t>;
using Vec2u = Vec2<uint32_t>;
using Vec2f = Vec2<float>;


// Compute intersection between a line and a circle
// Returns first intersection of line leading from `origin` in `direction`
// with circle at `center` with `radius`, or INFINITY if there is no such
// intersection.
template <typename T>
float line_circle_intersection(const Vec2<T> &origin,
                               const Vec2<T> &direction,
                               const Vec2<T> &center,
                               float radius)
{
    float a = direction.x * direction.x + direction.y * direction.y;
    auto sphere_dir = origin - center;
    float b = 2.0f * (sphere_dir.x * direction.x + sphere_dir.y * direction.y);
    float c = (sphere_dir.x * sphere_dir.x + sphere_dir.y * sphere_dir.y) - radius * radius;

    float discriminant = b*b - 4*a*c;
    if (discriminant < 0)
        return INFINITY;

    discriminant = std::sqrt(discriminant);
    const float t = (-b - discriminant) / (2*a);
    if (t >= 0)
        return t;
    else
        return INFINITY;
}


/// Compute distance from a point to an infinite line
/// \param point            The point
/// \param line_p1, line_p2 Two points on an infinite line
/// \returns                Distance in same units
template <typename T>
T dist_point_to_line(const Vec2<T>& point, const Vec2<T>& line_p1, const Vec2<T>& line_p2)
{
    const auto a = (line_p2.x - line_p1.x) * (line_p1.y - point.y) - (line_p1.x - point.x) * (line_p2.y - line_p1.y);
    return static_cast<T>(std::abs(cast_to_numeric(a))) / line_p1.dist(line_p2);
}


} // namespace xci::core

#endif // include guard
