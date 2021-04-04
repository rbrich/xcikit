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
    auto& parser = m_ctx.interpreter.parser();
    auto& compiler = m_ctx.interpreter.compiler();
    auto& machine = m_ctx.interpreter.machine();

    m_ctx.interpreter.configure(m_opts.compiler_flags);

    try {
        if (m_opts.with_std_lib && !m_ctx.std_module) {
            auto f = m_vfs.read_file("script/std.fire");
            auto content = f.content();
            m_ctx.std_module = m_ctx.interpreter.build_module("std", content->string_view());
        }

        // parse
        ast::Module ast;
        parser.parse(line, ast);

        if (m_opts.print_raw_ast) {
            t.stream() << "Raw AST:" << endl << dump_tree << ast << endl;
        }

        // compile
        std::string module_name = m_ctx.input_number >= 0 ? format("input_{}", m_ctx.input_number) : "<input>";
        auto module = std::make_unique<Module>(module_name);
        module->add_imported_module(BuiltinModule::static_instance());
        if (m_ctx.std_module)
            module->add_imported_module(*m_ctx.std_module);
        for (auto& m : m_ctx.input_modules)
            module->add_imported_module(*m);
        auto func_name = m_ctx.input_number == -1 ? "_" : "_" + std::to_string(m_ctx.input_number);
        auto func = std::make_unique<Function>(*module, module->symtab());
        compiler.compile(*func, ast);

        // print AST with Compiler modifications
        if (m_opts.print_ast) {
            t.stream() << "Processed AST:" << endl << dump_tree << ast << endl;
        }

        // print symbol table
        if (m_opts.print_symtab) {
            t.stream() << "Symbol table:" << endl << module->symtab() << endl;
        }

        // print compiled module content
        if (m_opts.print_module) {
            t.stream() << "Module content:" << endl << *module << endl;
        }

        // stop if we were only processing the AST, without actual compilation
        if ((m_opts.compiler_flags & Compiler::MandatoryMask) != 0)
            return false;

        BytecodeTracer tracer(machine, t);
        tracer.setup(m_opts.print_bytecode, m_opts.trace_bytecode);

        machine.call(*func, [&](const Value& invoked) {
            if (!invoked.is_void()) {
                t.print("{t:bold}{fg:yellow}{}{t:normal}\n", invoked);
            }
        });

        // returned value of last statement
        auto result = machine.stack().pull(func->effective_return_type());
        if (m_ctx.input_number != -1) {
            // REPL mode
            if (!result->is_void()) {
                t.print("{t:bold}{fg:magenta}{}:{} = {fg:default}{}{t:normal}\n",
                        func_name, result->type_info(), *result);
            }
            // save result as function `_<N>` in the module
            auto func_idx = module->add_function(move(func));
            module->symtab().add({move(func_name), Symbol::Function, func_idx});

            m_ctx.input_modules.push_back(move(module));
        } else {
            // single input mode
            if (!result->is_void()) {
                t.print("{t:bold}{}{t:normal}\n", *result);
            }
        }
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
