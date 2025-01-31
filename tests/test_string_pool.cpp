// test_string_pool.cpp created on 2023-09-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch_test_macros.hpp>

#include <xci/core/container/StringPool.h>

using namespace xci::core;


static void check_add_retrieve(StringPool& pool, std::string_view str) {
    auto id = pool.add(str);
    CHECK(pool.view(id) == str);
}


TEST_CASE( "Add and retrive strings", "[StringPool]" )
{
    StringPool pool;

    CHECK(pool.add("") == StringPool::empty_string);

    // This depends on little endian order
    CHECK(pool.add("T") == 84u);
    CHECK(pool.add("abc") == (97u | 98u << 8 | 99u << 16));

    check_add_retrieve(pool, "");
    check_add_retrieve(pool, "_");
    check_add_retrieve(pool, "_0");
    check_add_retrieve(pool, "123");
    check_add_retrieve(pool, "1234");

    // Everything above is stored directly in StringPool::Id (SSO)
    CHECK(pool.occupancy() == 0);

    check_add_retrieve(pool, "§§§§");  // out of 7bit ASCII range, cannot store in Id
    CHECK(pool.occupancy() == 1);

    auto id = pool.add("interned string");
    CHECK(pool.view(id) == "interned string");
    CHECK(pool.add("interned string") == id);  // found previous

    check_add_retrieve(pool, "abcde");
    check_add_retrieve(pool, "Hello world!");
    check_add_retrieve(pool, "Lorem ipsum dolor sit amet");

    CHECK(pool.add("interned string") == id);  // still found previous

    CHECK(pool.occupancy() == 5);
}


TEST_CASE( "Fill 1000 strings", "[StringPool]" )
{
    StringPool pool;
    for (int i = 0; i != 1000; ++i) {
        auto str = "string number " + std::to_string(i);
        check_add_retrieve(pool, str);
    }
    CHECK(pool.occupancy() == 1000);  // all strings are longer than 3 chars
}
