// demo_stack.cpp created on 2019-12-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/Stack.h>
#include <xci/core/log.h>
#include <deque>
#include <iostream>

using namespace xci::core;
using namespace std;

int main()
{
    // contained type doesn't matter:
    log_info("sizeof(deque<int>)    = {}", sizeof(std::deque<int>));
    log_info("sizeof(deque<string>) = {}", sizeof(deque<string>));
    log_info("sizeof(Stack<int>)    = {}", sizeof(Stack<int>));
    log_info("sizeof(Stack<string>) = {}", sizeof(Stack<string>));

    log_info("Stack of 1 int");
    {
        Stack<int> d;
        d.push_back(1);
        d.alloc_info(cerr);
    }

    log_info("Stack of 10000 ints");
    {
        Stack<int> d;
        for (int i = 0; i < 10000; ++i)
            d.push_back(i);
        d.alloc_info(cerr);
    }

    log_info("Stack of 10000 ints (reserved)");
    {
        Stack<int> d(10000);
        for (int i = 0; i < 10000; ++i)
            d.push_back(i);
        d.alloc_info(cerr);
    }
}
