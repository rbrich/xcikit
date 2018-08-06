// test_graphics.cpp created on 2018-08-04, part of XCI toolkit
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "catch.hpp"

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
