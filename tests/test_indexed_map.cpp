// test_indexed_map.cpp created on 2020-03-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch_test_macros.hpp>

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
    CHECK(map.size() == 4);

    // Moving invalidates (clears) original map
    IndexedMap<std::string> map2 = std::move(map);
    CHECK(map.empty());  // NOLINT
    CHECK(map.capacity() == 0);  // NOLINT
    CHECK(map2.size() == 4);

    map2.clear();
    CHECK(map2.empty());
    CHECK(map2.capacity() == 0);
}


TEST_CASE( "Walk indexed map", "[IndexedMap]" )
{
    IndexedMap<std::string> map;
    for (auto i = 0; i != 300; ++i) {
        map.add(std::to_string(i));
    }
    size_t i = 0;
    for (const auto& item : map) {
        CHECK(item == std::to_string(i));
        ++i;
    }
    const IndexedMap<std::string>& c_map = map;
    i = 0;
    for (const auto& item : c_map) {
        CHECK(item == std::to_string(i));
        ++i;
    }
}
