// test_math.cpp created on 2019-05-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch_test_macros.hpp>

#include <xci/math/Vec2.h>
#include <xci/math/Vec3.h>
#include <xci/math/Vec4.h>
#include <xci/math/Mat2.h>
#include <xci/math/Mat3.h>
#include <xci/math/Mat4.h>
#include <xci/math/Rect.h>
#include <xci/math/geometry.h>
#include <xci/math/transform.h>

#include <sstream>

using namespace xci;


TEST_CASE( "Vec2", "[math]" )
{
    Vec2f vf1 {3.0f, 4.0f};
    CHECK(vf1.length() == 5.0f);
    CHECK(vf1.norm() == Vec2f{0.6f, 0.8f});
    CHECK(vf1.dot(Vec2f{2.0f, -1.0f}) == 2.0f);

    Vec2i vi1 {3, 4};
    CHECK(vi1.length() == 5);

    Vec3f v3f {3.0f, 4.0f, 12.0f};
    CHECK(v3f.length() == 13.0f);
    CHECK(v3f.vec2(2, 0) == Vec2f(12.0f, 3.0f));
}


TEST_CASE( "Mat2", "[math]" )
{
    CHECK(Mat2f::identity().determinant() == 1.0f);
}


TEST_CASE( "Mat3", "[math]" )
{
    CHECK(Mat3f::identity().determinant() == 1.0f);
}


TEST_CASE( "Mat4", "[math]" )
{
    CHECK(Mat4f::identity().determinant() == 1.0f);
    auto m = Mat4f::rot_y(0.5f, 0.5f, {1.0f, 2.0f, 3.0f});
    CHECK(m * m.inverse() == Mat4f::identity());
}

template <typename T>
static std::string to_str(const Mat4<T>& m) {
    std::ostringstream s;
    s << m;
    return s.str();
}

TEST_CASE( "transform", "[math]" )
{
    CHECK(to_str(perspective_projection(1.2f, 4.0f / 3.0f, 1.0f, 1001.0f))
          == to_str(Mat4f{
                     1.09627, 0,       0,       0,
                     0,       1.4617,  0,       0,
                     0,       0,      -1.002,  -1,
                     0,       0,      -2.002,   0,
             }));
}
