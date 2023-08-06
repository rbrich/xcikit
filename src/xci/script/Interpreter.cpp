// Interpreter.cpp created on 2019-06-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
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
    const auto configured_flags = m_compiler.flags();
    m_compiler.set_flags(configured_flags | Compiler::Flags::Mandatory);
    m_compiler.compile(module->get_main_scope(), ast);
    m_compiler.set_flags(configured_flags);

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


TypedValue Interpreter::eval(size_t mod_idx, SourceId source_id, const InvokeCallback& cb)
{
    // parse
    ast::Module ast;
    m_parser.parse(source_id, ast);

    // compile
    auto& scope = m_module_manager.get_module(mod_idx)->get_main_scope();
    m_compiler.compile(scope, ast);

    // execute
    auto& fn = scope.function();
    m_machine.call(fn, cb);

    // get result from stack
    auto ti = fn.effective_return_type();
    return m_machine.stack().pull_typed(ti);
}


TypedValue Interpreter::eval(std::shared_ptr<Module> mod, std::string input, const Interpreter::InvokeCallback& cb)
{
    auto module_name = mod->name();
    auto src_id = m_source_manager.add_source(module_name, std::move(input));
    auto mod_idx = m_module_manager.replace_module(module_name, std::move(mod));
    return eval(mod_idx, src_id, cb);
}


TypedValue Interpreter::eval(std::string input, bool import_std, const Interpreter::InvokeCallback& cb)
{
    auto module_name = fmt::format("<input{}>", m_input_num++);
    auto src_id = m_source_manager.add_source(module_name, std::move(input));
    auto mod_idx = m_module_manager.replace_module(module_name);
    auto main = m_module_manager.get_module(mod_idx);
    main->import_module("builtin");
    if (import_std) {
        main->import_module("std");
    }
    return eval(mod_idx, src_id, cb);
}


} // namespace xci::script
