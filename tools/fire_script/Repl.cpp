// Repl.cpp.c created on 2021-03-16 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Repl.h"
#include "BytecodeTracer.h"

#include <xci/script/Error.h>
#include <xci/script/Value.h>
#include <xci/script/Builtin.h>
#include <xci/script/dump.h>

#include <fmt/core.h>
#include <range/v3/view/reverse.hpp>

#include <iostream>

namespace xci::script::tool {

using ranges::cpp20::views::reverse;
using std::endl;


bool Repl::evaluate(const std::string& module_name, std::string module_source, EvalMode mode)
{
    core::TermCtl& t = m_ctx.term_out;
    auto& source_manager = m_ctx.interpreter.source_manager();
    auto& parser = m_ctx.interpreter.parser();
    auto& compiler = m_ctx.interpreter.compiler();

    m_ctx.interpreter.configure(m_opts.compiler_flags);

    auto src_id = source_manager.add_source(module_name, std::move(module_source));

    try {
        // parse
        ast::Module ast;
        parser.parse(src_id, ast);

        if (m_opts.print_raw_ast) {
            t.stream() << "Raw AST:" << endl << dump_tree << ast << endl;
        }

        // create new module for the input
        auto module = prepare_module(module_name);

        // add main function to the module
        auto fn_idx = module->add_function(Function{*module, module->symtab()}).index;
        assert(fn_idx == 0);
        auto& fn = module->get_function(fn_idx);

        // compile
        bool is_compiled = compiler.compile(fn, ast);
        if (!is_compiled) {
            // We're only processing the AST, without actual compilation
            mode = EvalMode::Preprocess;
        }

        // print AST with Compiler modifications
        if (m_opts.print_ast) {
            t.stream() << "Processed AST:" << endl << dump_tree << ast << endl;
        }

        bool res = evaluate_module(*module, mode);

        if (mode == EvalMode::Compile || mode == EvalMode::Repl)
            m_ctx.input_modules.push_back(move(module));

        return res;
    } catch (const ScriptError& e) {
        print_error(e);
        return false;
    }
}


std::shared_ptr<Module> Repl::prepare_module(const std::string& module_name)
{
    auto module = std::make_shared<Module>(m_ctx.interpreter.module_manager(), module_name);
    module->import_module("builtin");

    if (m_opts.with_std_lib) {
        module->import_module("std");
    }

    for (auto& m : m_ctx.input_modules)
        module->add_imported_module(m);

    return module;
}


bool Repl::evaluate_module(Module& module, EvalMode mode)
{
    core::TermCtl& t = m_ctx.term_out;
    auto& machine = m_ctx.interpreter.machine();

    // print symbol table
    if (m_opts.print_symtab) {
        t.stream() << "Symbol table:" << endl << module.symtab() << endl;
    }

    // print compiled module content
    if (m_opts.print_module || m_opts.print_module_verbose || m_opts.print_module_verbose_ast) {
        auto s = t.stream();
        if (m_opts.print_module_verbose)
            s << dump_module_verbose;
        if (m_opts.print_module_verbose_ast)
            s << dump_module_verbose << dump_tree;
        s << "Module content:" << endl << module << endl;
    }

    if (mode == EvalMode::Preprocess || mode == EvalMode::Compile)
        return true;

    BytecodeTracer tracer(machine, t);
    tracer.setup(m_opts.print_bytecode, m_opts.trace_bytecode);

    try {
        constexpr unsigned int main_fn_idx = 0;
        auto& main_fn = module.get_function(main_fn_idx);
        machine.call(main_fn, [&](TypedValue&& invoked) {
            if (!invoked.is_void()) {
                t.sanitize_newline();
                t.print("{t:bold}{fg:yellow}{}{t:normal}\n", invoked);
            }
            invoked.decref();
        });
        t.sanitize_newline();

        // returned value of last statement
        auto result = machine.stack().pull_typed(main_fn.effective_return_type());
        if (mode == EvalMode::Repl) {
            // REPL mode
            const auto& module_name = module.name();
            if (!result.is_void()) {
                t.print("{t:bold}{fg:magenta}{}:{} = {fg:default}{}{t:normal}\n",
                        module_name, result.type_info(), result);
            }
            // add symbol for main function, so it will be visible by following input,
            // which imports this module
            module.symtab().add({module_name, Symbol::Function, main_fn_idx});
        } else {
            // single input mode
            assert(mode == EvalMode::SingleInput);
            if (!result.is_void()) {
                t.print("{t:bold}{}{t:normal}\n", result);
            }
        }
        result.decref();
        return true;
    } catch (const ScriptError& e) {
        print_error(e);
        return false;
    }
}


void Repl::print_error(const ScriptError& e)
{
    core::TermCtl& t = m_ctx.term_out;

    if (!e.trace().empty()) {
        int i = 0;
        for (const auto& frame : e.trace() | reverse) {
            t.stream()
                << "  #" << i++
                << " " << frame.function_name
                << '\n';
        }
    }
    if (!e.file().empty())
        t.stream() << e.file() << ": ";
    t.stream() << t.red().bold() << "Error: " << e.what() << t.normal();
    if (!e.detail().empty())
        t.stream() << endl << t.magenta() << e.detail() << t.normal();
    t.stream() << endl;
}


}  // namespace xci::script::tool
