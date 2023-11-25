// geometry.h created on 2018-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_MATH_GEOMETRY_H
#define XCI_MATH_GEOMETRY_H

#include "Vec2.h"
#include <cmath>

namespace xci {


/// Compute intersection between a line and a circle
/// Returns first intersection of line leading from `origin` in `direction`
/// with circle at `center` with `radius`, or INFINITY if there is no such
/// intersection.
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


} // namespace xci

#endif // include guard
