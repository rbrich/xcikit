// Interpreter.h created on 2019-06-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_INTERPRETER_H
#define XCI_SCRIPT_INTERPRETER_H

#include "Source.h"
#include "ModuleManager.h"
#include "Parser.h"
#include "Compiler.h"
#include "Machine.h"
#include <string_view>

namespace xci::script {


/// High level interpreter

class Interpreter {
public:
    explicit Interpreter(const core::Vfs& vfs, Compiler::Flags flags = Compiler::Flags::Default);

    // `flags` are Compiler::Flags
    void configure(Compiler::Flags flags) { m_compiler.set_flags(flags); }

    // Build a module.
    // Ignores mandatory Compiler::Flags, respects only optimization flags
    std::shared_ptr<Module> build_module(const std::string& name, SourceId source_id);

    using InvokeCallback = Machine::InvokeCallback;
    TypedValue eval(Index mod_idx, SourceId source_id, const InvokeCallback& cb = Machine::no_invoke_cb);
    TypedValue eval(std::shared_ptr<Module> mod, std::string input, const InvokeCallback& cb = Machine::no_invoke_cb);
    TypedValue eval(std::string input, bool import_std = true, const InvokeCallback& cb = Machine::no_invoke_cb);

    // low-level component access
    SourceManager& source_manager() { return m_source_manager; }
    ModuleManager& module_manager() { return m_module_manager; }
    Parser& parser() { return m_parser; }
    Compiler& compiler() { return m_compiler; }
    Machine& machine() { return m_machine; }

private:
    SourceManager m_source_manager;
    ModuleManager m_module_manager;  // import modules by name, once
    Parser m_parser { m_source_manager };
    Compiler m_compiler;
    Machine m_machine;
    int m_input_num = 0;
};


} // namespace xci::script

#endif // include guard
