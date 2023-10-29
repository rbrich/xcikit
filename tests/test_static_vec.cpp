// test_static_vec.cpp created on 2023-09-28 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch_test_macros.hpp>

#include <xci/core/container/StaticVec.h>
#include <string>

using namespace xci::core;

TEST_CASE( "StringVec<int>", "[StringVec]" )
{
    constexpr size_t size = 3;
    StaticVec<int> vec(size);
    CHECK(!vec.empty());
    CHECK(vec.size() == size);
    CHECK(vec.front() == 0);
    vec[0] = 1;
    vec[1] = 2;
    vec[2] = 3;
    CHECK(vec.front() == 1);
    CHECK(vec[1] == 2);
    CHECK(vec.back() == 3);

    vec.reset(10);
    CHECK(vec.front() == 0);
}

TEST_CASE( "Iterators", "[ChunkedStack]" )
{
    StaticVec<std::string> vec(300);
    for (auto i = 0; i != 300; ++i) {
        vec[i] = std::to_string(i);
    }
    size_t i = 0;
    for (const auto& item : vec) {
        CHECK(item == std::to_string(i));
        ++i;
    }
    const StaticVec<std::string>& c_vec = vec;
    i = 0;
    for (const auto& item : c_vec) {
        CHECK(item == std::to_string(i));
        ++i;
    }
}


TEST_CASE( "Moved out", "[ChunkedStack]" )
{
    StaticVec<int> vec(3);
    vec[0] = 1;
    vec[1] = 2;
    vec[2] = 3;
    StaticVec<int> vec2 = std::move(vec);
    CHECK(vec.size() == 0);  // NOLINT, intentional check after move
    REQUIRE(vec2.size() == 3);
    CHECK(vec2.front() == 1);
    CHECK(vec2[1] == 2);
    CHECK(vec2.back() == 3);
}
