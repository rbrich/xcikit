// Interpreter.cpp created on 2019-06-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Interpreter.h"
#include "Builtin.h"

#include <utility>
#include <fmt/core.h>

namespace xci::script {


Interpreter::Interpreter(const core::Vfs& vfs, Compiler::Flags flags)
    : m_module_manager(vfs, *this),
      m_compiler(flags)
{}


std::shared_ptr<Module> Interpreter::build_module(const std::string& name, SourceId source_id)
{
    // setup module
    auto module = std::make_shared<Module>(m_module_manager, name);
    module->import_module("builtin");

    // parse
    ast::Module ast;
    m_parser.parse(source_id, ast);

    // compile
    Function func {*module, module->symtab(), nullptr};
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


TypedValue Interpreter::eval(Module& module, SourceId source_id, const InvokeCallback& cb)
{
    // parse
    ast::Module ast;
    m_parser.parse(source_id, ast);

    // compile
    auto source_name = m_source_manager.get_source(source_id).name();
    auto& symtab = module.symtab().add_child(source_name);
    auto fn_idx = module.add_function(Function{module, symtab, nullptr}).index;
    auto& fn = module.get_function(fn_idx);
    m_compiler.compile(fn, ast);

    // execute
    m_machine.call(fn, cb);

    // get result from stack
    auto ti = fn.effective_return_type();
    return m_machine.stack().pull_typed(ti);
}


TypedValue Interpreter::eval(Module& module, std::string input, const Interpreter::InvokeCallback& cb)
{
    auto src_id = m_source_manager.add_source(
            fmt::format("<input{}>", m_input_num++),
            std::move(input));

    return eval(module, src_id, cb);
}


TypedValue Interpreter::eval(std::string input, const Interpreter::InvokeCallback& cb)
{
    Module main(m_module_manager);
    main.import_module("builtin");
    return eval(main, std::move(input), cb);
}


} // namespace xci::script
