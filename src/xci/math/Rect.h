// Rect.h created on 2018-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_MATH_RECT_H
#define XCI_MATH_RECT_H

#include "Vec2.h"

namespace xci {


template <typename T>
struct Rect {
    T x;  // left
    T y;  // top
    T w;  // width
    T h;  // height

    constexpr Rect() : x(0), y(0), w(0), h(0) {}
    constexpr Rect(T x, T y, T w, T h) : x(x), y(y), w(w), h(h) {}
    constexpr Rect(const Vec2<T>& pos, const Vec2<T>& size) : x(pos.x), y(pos.y), w(size.x), h(size.y) {}

    constexpr bool contains(const Vec2<T>& point) const {
        return point.x >= x && point.x <= x + w &&
               point.y >= y && point.y <= y + h;
    }
    //bool intersects(const Rect<T>& rectangle) const;

    constexpr Rect<T> union_(const Rect<T>& other) const {
        auto l = std::min(x, other.x);
        auto t = std::min(y, other.y);
        auto r = std::max(right(), other.right());
        auto b = std::max(bottom(), other.bottom());
        return {l, t, r-l, b-t};
    }

    constexpr Rect<T> intersection(const Rect<T>& other) const {
        auto l = std::max(x, other.x);
        auto t = std::max(y, other.y);
        auto r = std::min(right(), other.right());
        auto b = std::min(bottom(), other.bottom());
        return {l, t, r-l, b-t};
    }

    constexpr Rect<T> enlarged(T radius) const {
        return {x - radius, y - radius, w + 2*radius, h + 2*radius};
    }

    constexpr Rect<T> moved(const Vec2<T>& offset) const {
        return {x + offset.x, y + offset.y, w, h };
    }

    // Extend Rect to contain `other`
    constexpr void extend(const Rect<T>& other) {
        *this = union_(other);
    }

    constexpr void crop(const Rect<T>& other) {
        *this = intersection(other);
    }

    // Enlarge Rect to all sides by `radius`
    constexpr void enlarge(T radius) {
        x -= radius;
        y -= radius;
        w += 2 * radius;
        h += 2 * radius;
    }

    // Enlarge Rect to all sides by `radius`
    constexpr void enlarge(const Vec2<T>& radius) {
        x -= radius.x;
        y -= radius.y;
        w += 2 * radius.x;
        h += 2 * radius.y;
    }

    constexpr T left() const { return x; }
    constexpr T top() const { return y; }
    constexpr T right() const { return x + w; }
    constexpr T bottom() const { return y + h; }

    constexpr Vec2<T> top_left() const { return {x, y}; }
    constexpr Vec2<T> size() const { return {w, h}; }
    constexpr Vec2<T> center() const { return {x + w/2, y + h/2}; }
};

template <typename T>
std::ostream& operator <<(std::ostream& s, Rect<T> r) {
    return s << "{" << r.x << ", " << r.y << ", "
                    << r.w << ", " << r.h << "}";
}

using Rect_i = Rect<int32_t>;
using Rect_u = Rect<uint32_t>;
using Rect_f = Rect<float>;


} // namespace xci

#endif // include guard
