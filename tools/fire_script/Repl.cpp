// Repl.cpp.c created on 2021-03-16 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Repl.h"
#include "Context.h"
#include "BytecodeTracer.h"

#include <xci/script/Error.h>
#include <xci/script/Value.h>
#include <xci/script/dump.h>

#include <iostream>

namespace xci::script::tool {

using std::endl;


bool Repl::evaluate(std::string_view line)
{
    core::TermCtl& t = m_ctx.term_out;
    auto& source_manager = m_ctx.interpreter.source_manager();
    auto& parser = m_ctx.interpreter.parser();
    auto& compiler = m_ctx.interpreter.compiler();
    auto& machine = m_ctx.interpreter.machine();

    m_ctx.interpreter.configure(m_opts.compiler_flags);

    std::string module_name = m_ctx.input_number >= 0 ? format("input_{}", m_ctx.input_number) : "<input>";
    auto src_id = source_manager.add_source(module_name, std::string(line));

    try {
        if (m_opts.with_std_lib && !m_ctx.std_module) {
            const char* path = "script/std.fire";
            auto f = m_vfs.read_file(path);
            auto content = f.content();
            auto file_id = source_manager.add_source(path, std::string(content->string_view()));
            m_ctx.std_module = m_ctx.interpreter.build_module("std", file_id);
        }

        // parse
        ast::Module ast;
        parser.parse(src_id, ast);

        if (m_opts.print_raw_ast) {
            t.stream() << "Raw AST:" << endl << dump_tree << ast << endl;
        }

        // create new module for the input
        auto module = std::make_unique<Module>(module_name);
        module->add_imported_module(BuiltinModule::static_instance());
        if (m_ctx.std_module)
            module->add_imported_module(*m_ctx.std_module);
        for (auto& m : m_ctx.input_modules)
            module->add_imported_module(*m);

        // add main function to the module as `_<N>`
        auto fn_name = m_ctx.input_number == -1 ? "_" : "_" + std::to_string(m_ctx.input_number);
        auto fn_idx = module->add_function(std::make_unique<Function>(*module, module->symtab()));
        auto& fn = module->get_function(fn_idx);

        // compile
        bool is_compiled = compiler.compile(fn, ast);

        // print AST with Compiler modifications
        if (m_opts.print_ast) {
            t.stream() << "Processed AST:" << endl << dump_tree << ast << endl;
        }

        // print symbol table
        if (m_opts.print_symtab) {
            t.stream() << "Symbol table:" << endl << module->symtab() << endl;
        }

        // print compiled module content
        if (m_opts.print_module || m_opts.print_module_verbose) {
            auto s = t.stream();
            if (m_opts.print_module_verbose)
                s << dump_module_verbose << dump_tree;
            s << "Module content:" << endl << *module << endl;
        }

        // stop if we were only processing the AST, without actual compilation
        if (!is_compiled)
            return false;

        BytecodeTracer tracer(machine, t);
        tracer.setup(m_opts.print_bytecode, m_opts.trace_bytecode);

        machine.call(fn, [&](TypedValue&& invoked) {
            if (!invoked.is_void()) {
                t.sanitize_newline();
                t.print("{t:bold}{fg:yellow}{}{t:normal}\n", invoked);
            }
            invoked.decref();
        });
        t.sanitize_newline();

        // returned value of last statement
        auto result = machine.stack().pull_typed(fn.effective_return_type());
        if (m_ctx.input_number != -1) {
            // REPL mode
            if (!result.is_void()) {
                t.print("{t:bold}{fg:magenta}{}:{} = {fg:default}{}{t:normal}\n",
                        fn_name, result.type_info(), result);
            }
            // add symbol for main function so it will be visible by following input,
            // which imports this module
            module->symtab().add({fn_name, Symbol::Function, fn_idx});
            m_ctx.input_modules.push_back(move(module));
        } else {
            // single input mode
            if (!result.is_void()) {
                t.print("{t:bold}{}{t:normal}\n", result);
            }
        }
        result.decref();
        return true;
    } catch (const ScriptError& e) {
        if (!e.file().empty())
            t.stream() << e.file() << ": ";
        t.stream() << t.red().bold() << "Error: " << e.what() << t.normal();
        if (!e.detail().empty())
            t.stream() << endl << t.magenta() << e.detail() << t.normal();
        t.stream() << endl;
        return false;
    }
}


}  // namespace xci::script::tool
