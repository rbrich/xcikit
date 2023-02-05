// test_graphics.cpp created on 2018-08-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch_test_macros.hpp>

#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>
#include <xci/core/log.h>

using namespace xci::core;
using namespace xci::graphics;
using namespace xci::graphics::unit_literals;


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


TEST_CASE( "Color from string", "[Color]" )
{
    CHECK( Color("black") == Color::Black() );
    CHECK( Color("White") == Color::White() );
    CHECK( Color("CYAN") == Color::Cyan() );
    CHECK( Color("#08f") == Color(0, 0x88, 0xff) );
    CHECK( Color("#08f7") == Color(0, 0x88, 0xff, 0x77) );
    CHECK( Color("#1234AB") == Color(0x12, 0x34, 0xAB) );
    CHECK( Color("#1234AB00") == Color(0x12, 0x34, 0xAB, 0) );

    // Invalid values => red color + log error
    Logger::default_instance(Logger::Level::None);
    CHECK( Color("UNKNOWN") == Color::Red() );
    CHECK( Color("#12345") == Color::Red() );
    CHECK( Color("#1234567") == Color::Red() );
    CHECK( Color("#123456789") == Color::Red() );
}


TEST_CASE( "Variant units", "[VariUnits]" )
{
    // Type is encoded in upper bits
    CHECK( VariUnits(0_fb).type() == VariUnits::Framebuffer );
    CHECK( VariUnits(0_px).type() == VariUnits::Screen );
    CHECK( VariUnits(0_vp).type() == VariUnits::Viewport );
    CHECK( VariUnits(1_fb).type() == VariUnits::Framebuffer );
    CHECK( VariUnits(2_px).type() == VariUnits::Screen );
    CHECK( VariUnits(3_vp).type() == VariUnits::Viewport );
    CHECK( VariUnits(-1_fb).type() == VariUnits::Framebuffer );
    CHECK( VariUnits(-2_px).type() == VariUnits::Screen );
    CHECK( VariUnits(-3_vp).type() == VariUnits::Viewport );

    // Value is preserved
    CHECK( VariUnits(0_fb).framebuffer() == 0_fb );
    CHECK( VariUnits(0_px).screen() == 0_px );
    CHECK( VariUnits(0_vp).viewport() == 0_vp );
    CHECK( VariUnits(4_fb).framebuffer() == 4_fb );
    CHECK( VariUnits(5_px).screen() == 5_px );
    CHECK( VariUnits(6_vp).viewport() == 6_vp );
    CHECK( VariUnits(-4_fb).framebuffer() == -4_fb );
    CHECK( VariUnits(-5_px).screen() == -5_px );
    CHECK( VariUnits(-6_vp).viewport() == -6_vp );

    // Limits (overflow is asserted, UB in release)
    CHECK( VariUnits(524287.95_fb).raw_storage() == 0x1fffffc0 );
    CHECK( VariUnits(-524287.99_fb).raw_storage() == -0x20000000 );
    CHECK( VariUnits(524287.95_px).raw_storage() == 0x3fffffc0 );
    CHECK( VariUnits(-524287.99_px).raw_storage() == -0x40000000 );
    CHECK( VariUnits(16383.9995_vp).raw_storage() == 0x7fffffc0 );
    CHECK( VariUnits(-16383.9999_vp).raw_storage() == int32_t(-0x80000000) );
}
