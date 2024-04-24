// bm_core.cpp created on 2018-08-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <benchmark/benchmark.h>
#include <xci/core/string.h>
#include <xci/core/container/StringPool.h>

using namespace xci::core;


static void bm_utf8_to_codepoint(benchmark::State& state) {
    std::string input = "人";
    for (auto _ : state) {
        auto res = utf8_codepoint(input.data());
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK(bm_utf8_to_codepoint);


static void bm_codepoint_to_utf8(benchmark::State& state) {
    char32_t input = 0x1F99E;
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            auto res = to_utf8(input);
            benchmark::DoNotOptimize(res);
        }
    }
}
BENCHMARK(bm_codepoint_to_utf8)->Range(8, 8<<10);


static void bm_string_pool_dup(benchmark::State& state)
{
    for (auto _ : state) {
        StringPool pool;
        for (int i = 0; i < state.range(0); ++i) {
            auto id = pool.add("Attack of the clones");
            benchmark::DoNotOptimize(id);
        }
    }
}
BENCHMARK(bm_string_pool_dup)->Range(8, 8<<10);


static void bm_string_pool_nodup(benchmark::State& state)
{
    for (auto _ : state) {
        StringPool pool;
        for (int i = 0; i < state.range(0); ++i) {
            std::string str = "string number ";
            str += std::to_string(i);
            auto id = pool.add(str);
            benchmark::DoNotOptimize(id);
        }
    }
}
BENCHMARK(bm_string_pool_nodup)->Range(8, 8<<10);


BENCHMARK_MAIN();
