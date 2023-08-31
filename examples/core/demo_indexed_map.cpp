// demo_indexed_map.cpp created on 2023-08-24 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/container/IndexedMap.h>
#include <xci/core/log.h>
#include <iostream>

using namespace xci::core;
using namespace xci::core::log;

int main()
{
    // contained type doesn't matter:
    info("sizeof(IndexedMap<int>)    = {}", sizeof(IndexedMap<int>));
    info("sizeof(IndexedMap<string>) = {}", sizeof(IndexedMap<std::string>));

    info("IndexedMap of 1 int");
    {
        IndexedMap<int> d;
        auto idx = d.add(1);
        info("elem index = {}", idx.index);
        info("elem tenant = {}", idx.tenant);
        info("size = {}", d.size());
        info("capacity = {}", d.capacity());
    }

    info("IndexedMap of 10000 ints");
    {
        IndexedMap<int> d;
        for (int i = 1; i < 10000; ++i)
            d.add(i);
        info("size = {}", d.size());
        info("capacity = {}", d.capacity());
    }
}
