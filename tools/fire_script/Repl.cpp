// Repl.cpp.c created on 2021-03-16 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Repl.h"
#include "BytecodeTracer.h"

#include <xci/script/Error.h>
#include <xci/script/Value.h>
#include <xci/script/Builtin.h>
#include <xci/script/dump.h>
#include <xci/core/ResourceUsage.h>

#include <fmt/core.h>
#include <range/v3/view/reverse.hpp>

#include <iostream>

namespace xci::script::tool {

using xci::core::ResourceUsage;
using ranges::cpp20::views::reverse;
using std::endl;


bool Repl::evaluate(const std::string& module_name, std::string module_source, EvalMode mode)
{
    core::TermCtl& t = m_ctx.term_out;
    auto& source_manager = m_ctx.interpreter.source_manager();
    auto& module_manager = m_ctx.interpreter.module_manager();
    auto& parser = m_ctx.interpreter.parser();
    auto& compiler = m_ctx.interpreter.compiler();

    m_ctx.interpreter.configure(m_opts.compiler_flags);

    auto src_id = source_manager.add_source(module_name, std::move(module_source));

    try {
        ResourceUsage rusage;

        // parse
        rusage.start_if(m_opts.print_rusage, "parsed");
        ast::Module ast;
        parser.parse(src_id, ast);
        rusage.stop();

        if (m_opts.print_raw_ast) {
            t.stream() << "Raw AST:" << endl << dump_tree << ast << endl;
        }

        // create new module for the input
        auto module = prepare_module(module_name);
        auto r = module_manager.add_module(module_name, module);
        assert(r != no_index); (void) r;

        // compile
        rusage.start_if(m_opts.print_rusage, "compiled");
        bool is_compiled = compiler.compile(module->get_main_scope(), ast);
        rusage.stop();
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
            m_ctx.input_modules.push_back(std::move(module));

        return res;
    } catch (const ScriptError& e) {
        print_error(e);
        return false;
    }
}


std::shared_ptr<Module> Repl::prepare_module(const std::string& module_name)
{
    auto module = std::make_shared<Module>(m_ctx.interpreter.module_manager(), module_name);
    ResourceUsage rusage;

    rusage.start_if(m_opts.print_rusage, "builtin imported");
    module->import_module("builtin");
    rusage.stop();

    if (m_opts.with_std_lib) {
        rusage.start_if(m_opts.print_rusage, "std imported");
        module->import_module("std");
        rusage.stop();
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
        t.stream() << "Symbol table:\n" << module.symtab() << '\n';
    }

    // print scopes
    if (m_opts.print_symtab) {
        t.write("Scope trees:\n");
        t.tab_set_all({8, 30, 30, 30, 30, 30, 30}).write();
        for (unsigned i = 0; i != module.num_scopes(); ++i) {
            t.stream() << '[' << i << "]\t" << module.get_scope(i) << '\n';
        }
        t.tab_set_every(8).write_nl();
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

    ResourceUsage rusage;
    try {
        auto& main_fn = module.get_main_function();
        rusage.start_if(m_opts.print_rusage, "executed");
        machine.call(main_fn, [&](TypedValue&& invoked) {
            if (!invoked.is_void()) {
                t.sanitize_newline();
                t.print("{t:bold}{fg:yellow}{}{t:normal}\n", invoked);
            }
            invoked.decref();
        });
        rusage.stop();
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
        } else {
            // single input mode
            assert(mode == EvalMode::SingleInput);
            if (!result.is_void()) {
                t.print("{t:bold}{}{t:normal}\n", result);
            }
        }
        result.decref();
        return true;
    } catch (const RuntimeError& e) {
        print_runtime_error(e);
        return false;
    } catch (const ScriptError& e) {
        print_error(e);
        return false;
    }
}


void Repl::print_error(const ScriptError& e)
{
    core::TermCtl& t = m_ctx.term_out;

    if (!e.file().empty())
        t.print("{}: ", e.file());
    t.print("{fg:red}{t:bold}Error: {}{t:normal}", e.what());
    if (!e.detail().empty())
        t.print("\n{fg:magenta}{}{t:normal}", e.detail());
    t.write_nl();
}


void Repl::print_runtime_error(const RuntimeError& e)
{
    core::TermCtl& t = m_ctx.term_out;

    if (!e.trace().empty()) {
        int i = 0;
        for (const auto& frame : e.trace() | reverse) {
            t.print("  #{} {}\n", i++, frame.function_name);
        }
    }

    print_error(e);
}


}  // namespace xci::script::tool
