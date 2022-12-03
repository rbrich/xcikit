// bm_script.cpp created on 2021-02-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <benchmark/benchmark.h>
#include <xci/script/Parser.h>
#include <xci/script/ast/fold_tuple.h>

using namespace xci::script;
using std::string;


struct SimpleParser {
    SourceManager src_man;
    Parser parser {src_man};
    SourceId src_id;

    SimpleParser(const std::string& input)
        : src_id(src_man.add_source("<input>", input)) {}

    ast::Module parse() {
        ast::Module ast;
        parser.parse(src_id, ast);
        return ast;
    }
};


static void bm_parser_tuple(benchmark::State& state) {
    std::string input = "0";
    for (int i = 1; i < state.range(0); ++i) {
        input += ',';
        input += std::to_string(i);
    }
    SimpleParser parser(input);
    for (auto _ : state) {
        ast::Module ast = parser.parse();
        fold_tuple(ast.body);
        benchmark::DoNotOptimize(ast);
    }
}
BENCHMARK(bm_parser_tuple)->Range(1, 1<<8);


static void bm_parser_list(benchmark::State& state) {
    std::string input = "[0";
    for (int i = 1; i < state.range(0); ++i) {
        input += ',';
        input += std::to_string(i);
    }
    input += ']';
    SimpleParser parser(input);
    for (auto _ : state) {
        ast::Module ast = parser.parse();
        fold_tuple(ast.body);
        benchmark::DoNotOptimize(ast);
    }
}
BENCHMARK(bm_parser_list)->Range(1, 1<<8);


static void bm_parser_function_params(benchmark::State& state) {
    std::string input = "f = fun a0:Int";
    for (int i = 1; i < state.range(0); ++i) {
        input += " a";
        input += std::to_string(i);
        input += ":Int";
    }
    input += " { 0 }";
    SimpleParser parser(input);
    for (auto _ : state) {
        benchmark::DoNotOptimize(parser.parse());
    }
}
BENCHMARK(bm_parser_function_params)->Range(1, 1<<8);


static void bm_parser_functions(benchmark::State& state) {
    std::string input = "f0 = fun a { a + 0 };";
    for (int i = 1; i < state.range(0); ++i) {
        input += " f";
        input += std::to_string(i);
        input += " = fun a { a + ";
        input += std::to_string(i);
        input += " };";
    }
    SimpleParser parser(input);
    for (auto _ : state) {
        benchmark::DoNotOptimize(parser.parse());
    }
}
BENCHMARK(bm_parser_functions)->Range(1, 1<<8);


static void bm_parser_toplevel_expr(benchmark::State& state) {
    std::string input = "42 ** 0 + 7;";
    for (int i = 1; i < state.range(0); ++i) {
        input += " 42 ** ";
        input += std::to_string(i);
        input += " + 7;";
    }
    SimpleParser parser(input);
    for (auto _ : state) {
        benchmark::DoNotOptimize(parser.parse());
    }
}
BENCHMARK(bm_parser_toplevel_expr)->Range(1, 1<<8);


BENCHMARK_MAIN();
