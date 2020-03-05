// bm_chunked_stack.cpp created on 2019-12-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <benchmark/benchmark.h>
#include <xci/core/container/ChunkedStack.h>
#include <deque>

using namespace xci::core;


static void bm_chunked_stack(benchmark::State& state)
{
    for (auto _ : state) {
        ChunkedStack<int> stack;
        for (int i = 0; i < state.range(0); ++i)
            stack.push(i);
    }
}
BENCHMARK(bm_chunked_stack)->Range(8, 8<<10);


static void bm_chunked_stack_reserve(benchmark::State& state)
{
    for (auto _ : state) {
        ChunkedStack<int> stack(state.range(0));
        for (int i = 0; i < state.range(0); ++i)
            stack.push(i);
    }
}
BENCHMARK(bm_chunked_stack_reserve)->Range(8, 8<<10);


static void bm_std_deque(benchmark::State& state)
{
    for (auto _ : state) {
        std::deque<int> stack;
        for (int i = 0; i < state.range(0); ++i)
            stack.push_back(i);
    }
}
BENCHMARK(bm_std_deque)->Range(8, 8<<10);


static void bm_chunked_stack_foreach(benchmark::State& state)
{
    ChunkedStack<int> stack;
    for (int i = 0; i < state.range(0); ++i)
        stack.push(i);

    for (auto _ : state) {
        for (auto x : stack)
            benchmark::DoNotOptimize(x);
    }
}
BENCHMARK(bm_chunked_stack_foreach)->Range(8, 8<<10);


static void bm_std_deque_foreach(benchmark::State& state)
{
    std::deque<int> stack;
    for (int i = 0; i < state.range(0); ++i)
        stack.push_back(i);

    for (auto _ : state) {
        for (auto x : stack)
            benchmark::DoNotOptimize(x);
    }
}
BENCHMARK(bm_std_deque_foreach)->Range(8, 8<<10);


static void bm_chunked_stack_pump(benchmark::State& state)
{
    for (auto _ : state) {
        ChunkedStack<int> stack;
        for (int p = 8; p >= 1; --p) {
            for (int i = 0; i < state.range(0) / p; ++i)
                stack.push(i);
            for (int i = 0; i < state.range(0) / p; ++i)
                stack.pop();
        }
    }
}
BENCHMARK(bm_chunked_stack_pump)->Range(8, 8<<10);


static void bm_std_deque_pump(benchmark::State& state)
{
    for (auto _ : state) {
        std::deque<int> stack;
        for (int p = 8; p >= 1; --p) {
            for (int i = 0; i < state.range(0) / p; ++i)
                stack.push_back(i);
            for (int i = 0; i < state.range(0) / p; ++i)
                stack.pop_back();
        }
    }
}
BENCHMARK(bm_std_deque_pump)->Range(8, 8<<10);


BENCHMARK_MAIN();
