// ReplCommand.cpp created on 2020-01-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ReplCommand.h"
#include "Context.h"
#include <xci/script/dump.h>
#include <xci/core/TermCtl.h>
#include <iostream>
#include <utility>

namespace xci::script::tool {

using namespace xci::core;
using std::cout;
using std::endl;


static void cmd_quit() {
    context().done = true;
}


static void cmd_help() {
    cout << ".q, .quit                                  quit" << endl;
    cout << ".h, .help                                  show all accepted commands" << endl;
    cout << ".dm, .dump_module [#|name]                 print contents of last compiled module (or module by index or by name)" << endl;
    cout << ".df, .dump_function [#|name] [#|module]    print contents of last compiled function (or function by index/name from specified module)" << endl;
    cout << ".di, .dump_info                            print info about interpreter attributes on this machine" << endl;
}


static void cmd_dump_info() {
    cout << "Bloat:" << endl;
    cout << "  sizeof(Function) = " << sizeof(Function) << endl;
    cout << "  sizeof(Function::CompiledBody) = " << sizeof(Function::CompiledBody) << endl;
    cout << "  sizeof(Function::GenericBody) = " << sizeof(Function::GenericBody) << endl;
    cout << "  sizeof(Function::NativeBody) = " << sizeof(Function::NativeBody) << endl;
}


static void print_module_header(const Module& module)
{
    cout << "Module \"" << module.name() << '"'
         << " (" << std::hex << intptr_t(&module.symtab()) << std::dec << ')' << endl;
}


const Module* ReplCommand::module_by_idx(size_t mod_idx) {
    TermCtl& t = TermCtl::stdout_instance();
    auto& ctx = context();

    if (mod_idx == size_t(-1)) {
        if (!ctx.std_module) {
            cout << t.red().bold() << "Error: std module not loaded"
                 << t.normal() << endl;
            return nullptr;
        }
        return ctx.std_module.get();
    }
    if (mod_idx == size_t(-2))
        return &BuiltinModule::static_instance();

    if (mod_idx == size_t(-3))
        return &m_module;

    if (mod_idx >= ctx.input_modules.size()) {
        cout << t.red().bold() << "Error: module index out of range: "
             << mod_idx << t.normal() << endl;
        return nullptr;
    }
    return ctx.input_modules[mod_idx].get();
}


const Module* ReplCommand::module_by_name(const std::string& mod_name) {
    size_t n = 0;
    for (const auto& m : context().input_modules) {
        if (m->name() == mod_name)
            return m.get();
        ++n;
    }

    auto& ctx = context();
    TermCtl& t = TermCtl::stdout_instance();

    if (mod_name == "std") {
        if (!ctx.std_module) {
            cout << t.red().bold() << "Error: std module not loaded"
                 << t.normal() << endl;
            return nullptr;
        }
        return ctx.std_module.get();
    }

    if (mod_name == "builtin")
        return &BuiltinModule::static_instance();

    if (mod_name == "." || mod_name == "cmd")
        return &m_module;

    cout << t.red().bold() << "Error: module not found: " << mod_name
         << t.normal() << endl;
    return nullptr;
}


void ReplCommand::dump_module(size_t mod_idx) {
    auto* module = module_by_idx(mod_idx);
    if (!module)
        return;
    print_module_header(*module);
    cout << *module << endl;
}


void ReplCommand::cmd_dump_module() {
    dump_module(context().input_modules.size() - 1);
}


void ReplCommand::cmd_dump_module(size_t mod_idx) {
    dump_module(mod_idx);
}


void ReplCommand::cmd_dump_module(std::string mod_name) {
    auto* module = module_by_name(mod_name);
    if (!module)
        return;
    print_module_header(*module);
    cout << *module << endl;
}


void ReplCommand::dump_function(const Module& module, size_t fun_idx) {
    TermCtl& t = TermCtl::stdout_instance();

    if (fun_idx >= module.num_functions()) {
        cout << t.red().bold() << "Error: function index out of range: "
             << fun_idx << t.normal() << endl;
        return;
    }
    const auto& function = module.get_function(fun_idx);

    print_module_header(module);
    cout << "Function [" << fun_idx << "] " << function.name() << ": "
         << function << endl;
}


void ReplCommand::cmd_dump_function() {
    TermCtl& t = TermCtl::stdout_instance();
    if (context().input_modules.empty()) {
        cout << t.red().bold() << "Error: no input modules available" << t.normal() << endl;
        return;
    }
    size_t mod_idx = context().input_modules.size() - 1;
    const auto& module = *context().input_modules[mod_idx];

    if (module.num_functions() == 0) {
        cout << t.red().bold() << "Error: no functions available" << t.normal() << endl;
        return;
    }

    dump_function(module, module.num_functions() - 1);
}


void ReplCommand::cmd_dump_function(std::string fun_name) {
    TermCtl& t = TermCtl::stdout_instance();
    if (context().input_modules.empty()) {
        cout << t.red().bold() << "Error: no input modules available" << t.normal() << endl;
        return;
    }
    size_t mod_idx = context().input_modules.size() - 1;
    const auto& module = *context().input_modules[mod_idx];

    for (size_t i = 0; i != module.num_functions(); ++i) {
        if (module.get_function(i).name() == fun_name) {
            dump_function(module, i);
            return;
        }
    }
    cout << t.red().bold() << "Error: function not found: " << fun_name
         << t.normal() << endl;
}


void ReplCommand::cmd_dump_function(std::string fun_name, std::string mod_name) {
    TermCtl& t = TermCtl::stdout_instance();

    // lookup module
    auto* module = module_by_name(mod_name);
    if (!module)
        return;

    // lookup function
    for (size_t i = 0; i != module->num_functions(); ++i) {
        if (module->get_function(i).name() == fun_name) {
            dump_function(*module, i);
            return;
        }
    }
    cout << t.red().bold() << "Error: function not found: " << fun_name
         << t.normal() << endl;
}


void ReplCommand::cmd_dump_function(size_t fun_idx)
{
    TermCtl& t = TermCtl::stdout_instance();
    if (context().input_modules.empty()) {
        cout << t.red().bold() << "Error: no modules available" << t.normal() << endl;
        return;
    }
    size_t mod_idx = context().input_modules.size() - 1;
    dump_function(*context().input_modules[mod_idx], fun_idx);
}


void ReplCommand::cmd_dump_function(size_t fun_idx, size_t mod_idx)
{
    auto* module = module_by_idx(mod_idx);
    if (!module)
        return;
    dump_function(*module, fun_idx);
}


ReplCommand::ReplCommand()
{
    m_interpreter.add_imported_module(m_module);
    add_cmd("quit", "q", cmd_quit);
    add_cmd("help", "h", cmd_help);
    add_cmd("dump_info", "di", cmd_dump_info);
    add_cmd("dump_module", "dm",
            [](void* self)
            { ((ReplCommand*)self)->cmd_dump_module(); },
            this);
    add_cmd("dump_module", "dm",
            [](void* self, int32_t i)
            { ((ReplCommand*)self)->cmd_dump_module(i); },
            this);
    add_cmd("dump_module", "dm",
            [](void* self, std::string s)
            { ((ReplCommand*)self)->cmd_dump_module(std::move(s)); },
            this);
    m_module.symtab().detect_overloads("dump_module");
    m_module.symtab().detect_overloads("dm");
    add_cmd("dump_function", "df",
            [](void* self)
            { ((ReplCommand*)self)->cmd_dump_function(); },
            this);
    add_cmd("dump_function", "df",
            [](void* self, std::string f)
            { ((ReplCommand*)self)->cmd_dump_function(std::move(f)); },
            this);
    add_cmd("dump_function", "df",
            [](void* self, std::string f, std::string m)
            { ((ReplCommand*)self)->cmd_dump_function(std::move(f), std::move(m)); },
            this);
    add_cmd("dump_function", "df",
            [](void* self, int32_t f)
            { ((ReplCommand*)self)->cmd_dump_function(f); },
            this);
    add_cmd("dump_function", "df",
            [](void* self, int32_t f, int32_t m)
            { ((ReplCommand*)self)->cmd_dump_function(f, m); },
            this);
    m_module.symtab().detect_overloads("dump_function");
    m_module.symtab().detect_overloads("df");
}


}  // namespace xci::script::tool
