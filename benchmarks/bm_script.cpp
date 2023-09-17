// bm_script.cpp created on 2021-02-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <benchmark/benchmark.h>
#include <xci/script/Parser.h>
#include <xci/script/Module.h>
#include <xci/script/Machine.h>
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
    std::string input = "f = fun (a0:Int";
    for (int i = 1; i < state.range(0); ++i) {
        input += ", a";
        input += std::to_string(i);
        input += ":Int";
    }
    input += ") { a0";
    for (int i = 1; i < state.range(0); ++i) {
        input += ", a";
        input += std::to_string(i);
    }
    input += " }";
    SimpleParser parser(input);
    for (auto _ : state) {
        auto mod = parser.parse();
        benchmark::DoNotOptimize(mod);
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
        auto mod = parser.parse();
        benchmark::DoNotOptimize(mod);
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
        auto mod = parser.parse();
        benchmark::DoNotOptimize(mod);
    }
}
BENCHMARK(bm_parser_toplevel_expr)->Range(1, 1<<8);


static void bm_script_basic_arith(benchmark::State& state) {
    Machine machine;
    Module mod;
    Function fn(mod, mod.symtab().add_child("fn"));
    fn.set_assembly();
    fn.asm_code().add_L1(Opcode::LoadStatic, mod.add_value(TypedValue(value::Int(42))));
    for (int i = 1; i < state.range(0); ++i) {
        fn.asm_code().add_L1(Opcode::LoadStatic, mod.add_value(TypedValue(value::Int(2))));
        fn.asm_code().add_B1(Opcode::Mul, 0x99);
        fn.asm_code().add_L1(Opcode::LoadStatic, mod.add_value(TypedValue(value::Int(12))));
        fn.asm_code().add_B1(Opcode::Sub, 0x99);
        fn.asm_code().add_L1(Opcode::LoadStatic, mod.add_value(TypedValue(value::Int(i))));
        fn.asm_code().add_B1(Opcode::Add, 0x99);
        fn.asm_code().add_L1(Opcode::LoadStatic, mod.add_value(TypedValue(value::Int(2))));
        fn.asm_code().add_B1(Opcode::Div, 0x99);
    }
    fn.asm_code().add(Opcode::Ret);
    fn.assembly_to_bytecode();

    for (auto _ : state) {
        machine.call(fn);
        auto result = machine.stack().pull_typed(fn.effective_return_type());
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(bm_script_basic_arith)->Range(1, 1<<8);


BENCHMARK_MAIN();
