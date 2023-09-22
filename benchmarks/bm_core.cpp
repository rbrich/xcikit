// bm_core.cpp created on 2018-08-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <benchmark/benchmark.h>
#include <xci/core/string.h>
#include <xci/core/container/StringPool.h>

using namespace xci::core;


static void bm_utf8_codepoint(benchmark::State& state) {
    std::string input = "人";
    for (auto _ : state)
        utf8_codepoint(input.data());
}
BENCHMARK(bm_utf8_codepoint);


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
