// test_graphics.cpp - created by Radek Brich on 2018-03-07

#include "catch.hpp"

#include <xci/graphics/Window.h>

using namespace xci::graphics;

TEST_CASE( "Window lifetime", "[Window]" )
{
    Window window;

    // can be moved
    Window window1 = std::move(window);
    Window window2( std::move(window1) );

    CHECK(window.impl_ptr() == nullptr);
    CHECK(window1.impl_ptr() == nullptr);
    CHECK(window2.impl_ptr() != nullptr);
}
