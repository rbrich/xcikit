// geometry.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_CORE_GEOMETRY_H
#define XCI_CORE_GEOMETRY_H

#include <cmath>
#include <algorithm>
#include <ostream>

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
    Vec2() : x(0), y(0) {}
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

public:
    T x;
    T y;
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
Vec2<T>& operator +=(Vec2<T>& lhs, const Vec2<T>& rhs) {
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}

template <typename T>
Vec2<T>& operator -=(Vec2<T>& lhs, const Vec2<T>& rhs) {
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
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


template <typename T>
struct Rect {
    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(T x, T y, T w, T h) : x(x), y(y), w(w), h(h) {}
    Rect(const Vec2<T>& pos, const Vec2<T>& size) : x(pos.x), y(pos.y), w(size.x), h(size.y) {}

    bool contains(const Vec2<T>& point) const {
        return point.x >= x && point.x <= x + w &&
               point.y >= y && point.y <= y + h;
    }
    //bool intersects(const Rect<T>& rectangle) const;

    Rect<T> union_(const Rect<T>& other) const {
        auto l = std::min(x, other.x);
        auto t = std::min(y, other.y);
        auto r = std::max(right(), other.right());
        auto b = std::max(bottom(), other.bottom());
        return {l, t, r-l, b-t};
    }

    Rect<T> intersection(const Rect<T>& other) const {
        auto l = std::max(x, other.x);
        auto t = std::max(y, other.y);
        auto r = std::min(right(), other.right());
        auto b = std::min(bottom(), other.bottom());
        return {l, t, r-l, b-t};
    }

    Rect<T> enlarged(T radius) const {
        return {x - radius, y - radius, w + 2*radius, h + 2*radius};
    }

    Rect<T> moved(const Vec2<T>& offset) const {
        return {x + offset.x, y + offset.y, w, h };
    }

    // Extend Rect to contain `other`
    void extend(const Rect<T>& other) {
        *this = union_(other);
    }

    void crop(const Rect<T>& other) {
        *this = intersection(other);
    }

    // Enlarge Rect to all sides by `radius`
    void enlarge(T radius) {
        x -= radius;
        y -= radius;
        w += 2 * radius;
        h += 2 * radius;
    }

    inline T left() const { return x; }
    inline T top() const { return y; }
    inline T right() const { return x + w; }
    inline T bottom() const { return y + h; }

    inline Vec2<T> top_left() const { return {x, y}; }
    inline Vec2<T> size() const { return {w, h}; }
    inline Vec2<T> center() const { return {x + w/2, y + h/2}; }

public:
    T x;  // left
    T y;  // top
    T w;  // width
    T h;  // height
};

template <typename T>
std::ostream& operator <<(std::ostream& s, Rect<T> r) {
    return s << "{" << r.x << ", " << r.y << ", "
                    << r.w << ", " << r.h << "}";
}

using Rect_i = Rect<int32_t>;
using Rect_u = Rect<uint32_t>;
using Rect_f = Rect<float>;



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
    float t = (-b - discriminant) / (2*a);
    if (t >= 0)
        return t;
    else
        return INFINITY;
}


} // namespace xci::core

#endif //XCI_CORE_GEOMETRY_H
