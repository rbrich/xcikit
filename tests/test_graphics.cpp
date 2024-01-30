// test_graphics.cpp created on 2018-08-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch_test_macros.hpp>

#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>
#include <xci/graphics/Texture.h>
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
    CHECK( VariUnits(0_fb).as_framebuffer() == 0_fb );
    CHECK( VariUnits(0_px).as_screen() == 0_px );
    CHECK( VariUnits(0_vp).as_viewport() == 0_vp );
    CHECK( VariUnits(4_fb).as_framebuffer() == 4_fb );
    CHECK( VariUnits(5_px).as_screen() == 5_px );
    CHECK( VariUnits(6_vp).as_viewport() == 6_vp );
    CHECK( VariUnits(-4_fb).as_framebuffer() == -4_fb );
    CHECK( VariUnits(-5_px).as_screen() == -5_px );
    CHECK( VariUnits(-6_vp).as_viewport() == -6_vp );

    // Limits (overflow is asserted, UB in release)
    CHECK( VariUnits(524287.95_fb).raw_storage() == 0x1fffffc0 );
    CHECK( VariUnits(-524287.99_fb).raw_storage() == -0x20000000 );
    CHECK( VariUnits(524287.95_px).raw_storage() == 0x3fffffc0 );
    CHECK( VariUnits(-524287.99_px).raw_storage() == -0x40000000 );
    CHECK( VariUnits(16383.9995_vp).raw_storage() == 0x7fffffc0 );
    CHECK( VariUnits(-16383.9999_vp).raw_storage() == int32_t(-0x80000000) );
}


TEST_CASE( "Texture mipmap levels", "[Texture]" )
{
    CHECK( mip_levels_for_size({0, 0}) == 0 );
    CHECK( mip_levels_for_size({1, 1}) == 1 );
    CHECK( mip_levels_for_size({2, 2}) == 2 );
    CHECK( mip_levels_for_size({3, 3}) == 2 );
    CHECK( mip_levels_for_size({4, 4}) == 3 );
    CHECK( mip_levels_for_size({7, 7}) == 3 );
    CHECK( mip_levels_for_size({8, 8}) == 4 );
    CHECK( mip_levels_for_size({255, 255}) == 8 );
    CHECK( mip_levels_for_size({256, 256}) == 9 );
    CHECK( mip_levels_for_size({1000, 1000}) == 10 );
    CHECK( mip_levels_for_size({1024, 1024}) == 11 );
    CHECK( mip_levels_for_size({1024, 255}) == 11 );
    CHECK( mip_levels_for_size({1, 1024}) == 11 );
}
