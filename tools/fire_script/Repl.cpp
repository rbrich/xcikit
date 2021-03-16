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

using std::cout;
using std::endl;


void Repl::print_intro(const char* version)
{
    auto & t = m_ctx.term_out;
    cout << t.format((const char*)u8"{t:bold}{fg:magenta}🔥 fire script{t:normal} {fg:magenta}v{}{t:normal}\n", version);
}


bool Repl::evaluate(const std::string& line, int input_number)
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
            cout << "Raw AST:" << endl << dump_tree << ast << endl;
        }

        // compile
        std::string module_name = input_number >= 0 ? format("input_{}", input_number) : "<input>";
        auto module = std::make_unique<Module>(module_name);
        module->add_imported_module(BuiltinModule::static_instance());
        if (m_ctx.std_module)
            module->add_imported_module(*m_ctx.std_module);
        for (auto& m : m_ctx.input_modules)
            module->add_imported_module(*m);
        auto func_name = input_number == -1 ? "_" : "_" + std::to_string(input_number);
        auto func = std::make_unique<Function>(*module, module->symtab());
        compiler.compile(*func, ast);

        // print AST with Compiler modifications
        if (m_opts.print_ast) {
            cout << "Processed AST:" << endl << dump_tree << ast << endl;
        }

        // print symbol table
        if (m_opts.print_symtab) {
            cout << "Symbol table:" << endl << module->symtab() << endl;
        }

        // print compiled module content
        if (m_opts.print_module) {
            cout << "Module content:" << endl << *module << endl;
        }

        // stop if we were only processing the AST, without actual compilation
        if ((m_opts.compiler_flags & Compiler::PPMask) != 0)
            return false;

        BytecodeTracer tracer(machine, t);
        tracer.setup(m_opts.print_bytecode, m_opts.trace_bytecode);

        machine.call(*func, [&](const Value& invoked) {
            if (!invoked.is_void()) {
                cout << t.bold().yellow() << invoked << t.normal() << endl;
            }
        });

        // returned value of last statement
        auto result = machine.stack().pull(func->effective_return_type());
        if (input_number != -1) {
            // REPL mode
            if (!result->is_void()) {
                cout << t.bold().magenta() << func_name << " = "
                     << t.normal()
                     << t.bold() << *result << t.normal() << endl;
            }
            // save result as function `_<N>` in the module
            auto func_idx = module->add_function(move(func));
            module->symtab().add({move(func_name), Symbol::Function, func_idx});

            m_ctx.input_modules.push_back(move(module));
        } else {
            // single input mode
            if (!result->is_void()) {
                cout << t.bold() << *result << t.normal() << endl;
            }
        }
        return true;
    } catch (const ScriptError& e) {
        if (!e.file().empty())
            cout << e.file() << ": ";
        cout << t.red().bold() << "Error: " << e.what() << t.normal();
        if (!e.detail().empty())
            cout << endl << t.magenta() << e.detail() << t.normal();
        cout << endl;
        return false;
    }
}


}  // namespace xci::script::tool
