// bm_core.cpp created on 2018-08-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <benchmark/benchmark.h>
#include <xci/core/string.h>

using namespace xci::core;


static void bm_utf8_codepoint(benchmark::State& state) {
    std::string input = "人";
    for (auto _ : state)
        utf8_codepoint(input.data());
}
BENCHMARK(bm_utf8_codepoint);


BENCHMARK_MAIN();
