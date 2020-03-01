// test_indexed_map.cpp created on 2020-03-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch.hpp>

#include <xci/core/container/IndexedMap.h>
#include <string>

using namespace xci::core;


TEST_CASE( "Sparse indexed map", "[IndexedMap]" )
{
    IndexedMap<std::string> map;

    CHECK(map.empty());
    CHECK(map.size() == 0);  // NOLINT
    CHECK(map.capacity() == 0);

    auto idx1 = map.emplace("no small string optimization please");
    map.add("foo");
    std::string x {"bar"};
    auto idx3 = map.add(x);
    CHECK(!map.empty());
    CHECK(map.size() == 3);
    CHECK(map.capacity() == 64);
    CHECK(map[idx3.index] == x);

    auto it = map.cbegin();
    auto prev = it++;
    CHECK(prev == map.cbegin());
    ++it;
    CHECK(*it == x);
    ++it;
    CHECK(it == map.cend());

    CHECK(map.remove(idx1));
    CHECK(!map.remove(idx1)); // already removed

    auto idx1b = map.emplace("hello");
    CHECK(idx1.index == idx1b.index);
    CHECK(idx1.tenant != idx1b.tenant);
    CHECK(!map.remove(idx1)); // invalid weak index

    map.clear();
    CHECK(map.empty());
    CHECK(map.capacity() == 0);
}
