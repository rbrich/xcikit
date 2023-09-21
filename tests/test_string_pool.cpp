// test_string_pool.cpp created on 2023-09-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch_test_macros.hpp>

#include <xci/core/container/StringPool.h>

using namespace xci::core;


void check_add_retrieve(StringPool& pool, const char* str) {
    auto id = pool.add(str);
    const char* got = pool.get(id);
    CHECK(std::string(str) == got);
}


TEST_CASE( "Add and retrive strings", "[StringPool]" )
{
    StringPool pool;

    CHECK(pool.add("") == StringPool::empty_string);

    // This depends on little endian order
    CHECK(pool.add("T") == 84u);
    CHECK(pool.add("abc") == (97u | 98u << 8 | 99u << 16));

    auto id = pool.add("interned string");
    CHECK(pool.get(id) == std::string("interned string"));
    CHECK(pool.add("interned string") == id);  // found previous

    check_add_retrieve(pool, "");
    check_add_retrieve(pool, "abcde");
    check_add_retrieve(pool, "Hello world!");
    check_add_retrieve(pool, "Lorem ipsum dolor sit amet");

    CHECK(pool.add("interned string") == id);  // still found previous
}


TEST_CASE( "Fill 1000 strings", "[StringPool]" )
{
    StringPool pool;
    for (int i = 0; i != 1000; ++i) {
        auto str = "string number " + std::to_string(i);
        check_add_retrieve(pool, str.c_str());
    }
    CHECK(pool.occupancy() == 1000);  // all strings are longer than 3 chars
}
