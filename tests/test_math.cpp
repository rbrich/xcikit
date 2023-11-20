// test_math.cpp created on 2019-05-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch_test_macros.hpp>

#include <xci/math/Vec2.h>

using namespace xci::core;


TEST_CASE( "Vec2", "[math]" )
{
    Vec2f vf1 {3.0, 4.0};
    CHECK(vf1.length() == 5.0);

    Vec2i vi1 {3, 4};
    CHECK(vi1.length() == 5);
}
