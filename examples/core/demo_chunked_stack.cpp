// demo_stack.cpp created on 2019-12-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/container/ChunkedStack.h>
#include <xci/core/log.h>
#include <deque>
#include <iostream>

using namespace xci::core;
using namespace xci::core::log;
using namespace std;

int main()
{
    // contained type doesn't matter:
    info("sizeof(deque<int>)    = {}", sizeof(std::deque<int>));
    info("sizeof(deque<string>) = {}", sizeof(deque<string>));
    info("sizeof(ChunkedStack<int>)    = {}", sizeof(ChunkedStack<int>));
    info("sizeof(ChunkedStack<string>) = {}", sizeof(ChunkedStack<string>));

    info("Stack of 1 int");
    {
        ChunkedStack<int> d;
        d.push_back(1);
        d.alloc_info(cerr);
    }

    info("Stack of 10000 ints");
    {
        ChunkedStack<int> d;
        for (int i = 0; i < 10000; ++i)
            d.push_back(i);
        d.alloc_info(cerr);
    }

    info("Stack of 10000 ints (reserved)");
    {
        ChunkedStack<int> d(10000);
        for (int i = 0; i < 10000; ++i)
            d.push_back(i);
        d.alloc_info(cerr);
    }
}
