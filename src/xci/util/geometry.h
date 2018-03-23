// geometry.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_UTIL_GEOMETRY_H
#define XCI_UTIL_GEOMETRY_H

#include <cmath>
#include <algorithm>

namespace xci {
namespace util {


template <typename T>
struct Vec2 {
    Vec2() : x(0), y(0) {}
    Vec2(T x, T y) : x(x), y(y) {}

    // Convert another type of vector (possibly foreign type)
    template <typename TVec>
    explicit Vec2(const TVec& other) : x(other.x), y(other.y) {}

    T length() const {
        return std::hypot(x, y);
    }

    const Vec2<T> norm() const {
        float l = length();
        return { x / l, y / l };
    }

    T dist(const Vec2<T>& other) const {
        return static_cast<T>(std::hypot(x - other.x, y - other.y));
    }

    T dist_squared(const Vec2<T>& other) const {
        auto dx = x - other.x;
        auto dy = y - other.y;
        return dx*dx + dy*dy;
    }

    T dist_taxicab(const Vec2<T>& other) const {
        auto dx = x - other.x;
        auto dy = y - other.y;
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
Vec2<T> operator -(const Vec2<T>& lhs, const Vec2<T>& rhs) {
    return Vec2<T>(lhs.x - rhs.x, lhs.y - rhs.y);
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

using Vec2i = Vec2<int>;
using Vec2u = Vec2<unsigned int>;
using Vec2f = Vec2<float>;


template <typename T>
struct Rect {
    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(T x, T y, T w, T h) : x(x), y(y), w(w), h(h) {}
    Rect(const Vec2<T>& pos, const Vec2<T>& size) : x(pos.x), y(pos.y), w(size.x), h(size.y) {}

    //bool contains(const Vec2<T>& point) const;
    //bool intersects(const Rect<T>& rectangle) const;

    Rect<T> union_(const Rect<T>& other) const {
        auto l = std::min(x, other.x);
        auto t = std::min(y, other.y);
        auto r = std::max(right(), other.right());
        auto b = std::max(bottom(), other.bottom());
        return {l, t, r-l, b-t};
    }

    // Extend Rect to contain `other`
    void extend(const Rect<T>& other) {
        *this = union_(other);
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

    bool empty() const { return x + y + w + h == 0; }

public:
    T x;  // left
    T y;  // top
    T w;  // width
    T h;  // height
};

using Rect_i = Rect<int>;
using Rect_u = Rect<unsigned int>;
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



}} // namespace xci::log

#endif //XCI_UTIL_GEOMETRY_H
