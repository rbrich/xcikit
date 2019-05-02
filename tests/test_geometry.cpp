// test_geometry.cpp created on 2019-05-01, part of XCI toolkit
// Copyright 2019 Radek Brich
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

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <xci/core/geometry.h>

using namespace xci::core;


TEST_CASE( "to_utf8", "[string]" )
{
    Vec2f vf1 {3.0, 4.0};
    CHECK(vf1.length() == 5.0);

    Vec2i vi1 {3, 4};
    CHECK(vi1.length() == 5);
}
