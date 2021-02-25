// test_chunked_stack.cpp created on 2019-12-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch.hpp>

#include <xci/core/container/ChunkedStack.h>
#include <string>

using namespace xci::core;

TEST_CASE( "ChunkedStack<int>", "[ChunkedStack]" )
{
    constexpr uint32_t init_cap = 10;
    ChunkedStack<int> stack(init_cap);

    stack.clear();  // clear empty stack -> NOOP
    CHECK(stack.empty());
    CHECK(stack.size() == 0);  // NOLINT: not same as empty()
    CHECK(stack.capacity() == init_cap);

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

    for (int i = 0; i < int(2*init_cap); ++i)
        stack.push(i);
    CHECK(stack.capacity() > init_cap);

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


TEST_CASE( "ChunkedStack<string>", "[ChunkedStack]" )
{
    constexpr uint32_t init_cap = 10;
    ChunkedStack<std::string> stack(init_cap);

    stack.clear();  // clear empty stack -> NOOP
    CHECK(stack.empty());
    CHECK(stack.size() == 0);  // NOLINT: not same as empty()
    CHECK(stack.capacity() == init_cap);

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


TEST_CASE( "ChunkedStack<struct>", "[ChunkedStack]" )
{
    struct TestT {
        std::string name;
        void* address = nullptr;

        explicit TestT(const char* name) : name(name) {}
    };
#ifdef __EMSCRIPTEN__
    static_assert(alignof(TestT) == 4);
#else
    static_assert(alignof(TestT) == 8);
#endif

    constexpr uint32_t init_cap = 10;
    ChunkedStack<TestT> stack(init_cap);

    stack.clear();  // clear empty stack -> NOOP
    CHECK(stack.empty());
    CHECK(stack.size() == 0);  // NOLINT: not same as empty()
    CHECK(stack.capacity() == init_cap);

    stack.emplace("no small string optimization please");
    stack.push(TestT{"bar"});
    TestT x{"third"};
    stack.push(x);
    CHECK(!stack.empty());
    CHECK(stack.size() == 3);
    CHECK(stack.capacity() == init_cap);

    auto it = stack.cbegin();
    ++it;
    auto prev = it++;
    CHECK(prev->name == "bar");
    CHECK(prev->address == nullptr);
    CHECK(it->name.length() == 5);
}


TEST_CASE( "Iterators", "[ChunkedStack]" )
{
    ChunkedStack<int> stack;
    CHECK(stack.begin() == stack.end());
    CHECK(stack.cbegin() == stack.cend());

    stack.push(1);
    CHECK(stack.begin() != stack.end());
    CHECK(stack.cbegin() != stack.cend());
    auto it = stack.begin();
    CHECK(++it == stack.end());
    auto cit = stack.cbegin();
    (void) cit++;
    CHECK(cit == stack.cend());
}
