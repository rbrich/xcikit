// Interpreter.cpp created on 2019-06-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Interpreter.h"

namespace xci::script {


Interpreter::Interpreter(Compiler::Flags flags)
    : m_compiler(flags)
{
    add_imported_module(BuiltinModule::static_instance());
}


std::unique_ptr<Module> Interpreter::build_module(const std::string& name, SourceId source_id)
{
    // setup module
    auto module = std::make_unique<Module>(name);
    module->add_imported_module(BuiltinModule::static_instance());

    // parse
    ast::Module ast;
    m_parser.parse(source_id, ast);

    // compile
    Function func {*module, module->symtab()};
    if (!m_compiler.compile(func, ast))
        return module;  // requested to only preprocess AST

    // sanity check (no AST references)
    for (Index idx = 0; idx != module->num_functions(); ++idx) {
        auto& fn = module->get_function(idx);
        if (fn.is_generic()) {
            assert(fn.is_ast_copied());
            fn.ensure_ast_copy();
        }
    }

    return module;
}


TypedValue Interpreter::eval(SourceId source_id, const InvokeCallback& cb)
{
    // parse
    ast::Module ast;
    m_parser.parse(source_id, ast);

    // compile
    auto source_name = m_source_manager.get_source(source_id).name();
    auto& symtab = m_main.symtab().add_child(source_name);
    Function func {m_main, symtab};
    m_compiler.compile(func, ast);

    // execute
    m_machine.call(func, cb);

    // get result from stack
    auto ti = func.effective_return_type();
    return m_machine.stack().pull_typed(ti);
}


TypedValue Interpreter::eval(std::string input, const Interpreter::InvokeCallback& cb)
{
    auto src_id = m_source_manager.add_source("<input>", input);
    return eval(src_id, cb);
}


} // namespace xci::script
