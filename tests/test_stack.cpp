// test_stack.cpp created on 2019-12-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <xci/core/Stack.h>
#include <string>

using namespace xci::core;

TEST_CASE( "Stack of ints", "[stack]" )
{
    Stack<int> stack;
    constexpr auto init_cap = Stack<int>::initial_capacity;

    stack.clear();  // clear empty stack -> NOOP
    CHECK(stack.empty());
    CHECK(stack.size() == 0);  // NOLINT: not same as empty()
    CHECK(stack.capacity() == 0);

    stack.emplace(-1);
    stack.push(-2);
    int x = -3;
    stack.push(x);
    CHECK(!stack.empty());
    CHECK(stack.size() == 3);
    CHECK(stack.capacity() == init_cap);

    stack.pop();
    stack.pop();
    stack.pop();
    CHECK(stack.empty());
    CHECK(stack.capacity() == init_cap);

    for (size_t i = 0; i < 2*init_cap; ++i)
        stack.push(i);
    CHECK(stack.capacity() == init_cap * 3);

    int expected = 0;
    for (int i : stack) {
        CHECK(i == expected);
        ++expected;
    }

    auto it = stack.begin();
    ++it;
    auto prev = it++;
    CHECK(*prev == 1);
    CHECK(*it == 2);

    // copy
    auto stack2 { stack };
    CHECK(stack2.capacity() == stack.size());
    CHECK(stack2.size() == stack.size());
    CHECK(stack2 == stack);

    stack.shrink_to_fit();
    CHECK(stack.size() == stack.capacity());
    stack.push(42);
    stack.shrink_to_fit();
    stack.shrink_to_fit();
    CHECK(stack.top() == 42);
    CHECK(stack.size() == stack.capacity());
    stack.pop();
    CHECK(stack.top() == 2*init_cap - 1);
}


TEST_CASE( "Stack of strings", "[stack]" )
{
    Stack<std::string> stack;
    constexpr auto init_cap = Stack<std::string>::initial_capacity;

    stack.clear();  // clear empty stack -> NOOP
    CHECK(stack.empty());
    CHECK(stack.size() == 0);  // NOLINT: not same as empty()
    CHECK(stack.capacity() == 0);

    stack.emplace("no small string optimization please");
    stack.push("bar");
    std::string x = "third";
    stack.push(x);
    CHECK(!stack.empty());
    CHECK(stack.size() == 3);
    CHECK(stack.capacity() == init_cap);

    auto it = stack.cbegin();
    ++it;
    auto prev = it++;
    CHECK(*prev == "bar");
    CHECK(it->length() == 5);
}
