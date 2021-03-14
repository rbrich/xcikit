// test_graphics.cpp created on 2018-08-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch.hpp>

#include <xci/graphics/Color.h>

using namespace xci::graphics;


TEST_CASE( "Float colors", "[Color]" )
{
    CHECK( Color(0.0, 0.0, 0.0) == Color::Black() );
    CHECK( Color(1.0, 1.0, 1.0) == Color::White() );
}


TEST_CASE( "Indexed colors", "[Color]" )
{
    CHECK( Color(0) == Color::Black() );
    CHECK( Color(16) == Color::Black() );
    CHECK( Color(18) == Color(0, 0, 135) );
    CHECK( Color(196) == Color::Red() );
    CHECK( Color(226) == Color::Yellow() );
    CHECK( Color(230) == Color(255, 255, 215) );
    CHECK( Color(231) == Color::White() );
    CHECK( Color(232) == Color(8, 8, 8) );
    CHECK( Color(254) == Color(228, 228, 228) );
    CHECK( Color(255) == Color(238, 238, 238) );
}
