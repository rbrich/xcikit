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


void ReplCommand::dump_module(size_t mod_idx) {
    // dump command module itself
    if (mod_idx == size_t(-1)) {
        cout << "Command module:" << endl << m_module << endl;
        return;
    }

    auto& ctx = context();
    TermCtl& t = TermCtl::stdout_instance();
    if (ctx.modules.empty()) {
        cout << t.red().bold() << "Error: no modules available" << t.normal() << endl;
        return;
    }
    if (mod_idx >= ctx.modules.size()) {
        cout << t.red().bold() << "Error: module index out of range: "
             << mod_idx << t.normal() << endl;
        return;
    }

    const auto& module = *ctx.modules[mod_idx];
    cout << "Module [" << mod_idx << "] " << module.name() << ":" << endl
         << module << endl;
}


void ReplCommand::cmd_dump_module() {
    dump_module(context().modules.size() - 1);
}


void ReplCommand::cmd_dump_module(size_t mod_idx) {
    dump_module(mod_idx);
}


void ReplCommand::cmd_dump_module(std::string mod_name) {
    size_t n = 0;
    for (const auto& m : context().modules) {
        if (m->name() == mod_name) {
            dump_module(n);
            return;
        }
        ++n;
    }

    // dump command module itself
    if (mod_name == "." || mod_name == "cmd") {
        dump_module(size_t(-1));
        return;
    }

    TermCtl& t = TermCtl::stdout_instance();
    cout << t.red().bold() << "Error: module not found: " << mod_name
         << t.normal() << endl;
}


void ReplCommand::dump_function(size_t mod_idx, size_t fun_idx) {
    auto& ctx = context();
    TermCtl& t = TermCtl::stdout_instance();

    if (mod_idx != size_t(-1) && mod_idx >= ctx.modules.size()) {
        cout << t.red().bold() << "Error: module index out of range: "
             << mod_idx << t.normal() << endl;
        return;
    }
    const auto& module = mod_idx == size_t(-1)? m_module : *ctx.modules[mod_idx];

    if (fun_idx >= module.num_functions()) {
        cout << t.red().bold() << "Error: function index out of range: "
             << mod_idx << t.normal() << endl;
        return;
    }
    const auto& function = module.get_function(fun_idx);

    cout << "Module [" << mod_idx << "] " << module.name() << ":" << endl
         << "Function [" << fun_idx << "] " << function.name() << ": "
         << function << endl;
}


void ReplCommand::cmd_dump_function() {
    TermCtl& t = TermCtl::stdout_instance();
    if (context().modules.empty()) {
        cout << t.red().bold() << "Error: no modules available" << t.normal() << endl;
        return;
    }
    size_t mod_idx = context().modules.size() - 1;
    const auto& module = *context().modules[mod_idx];

    if (module.num_functions() == 0) {
        cout << t.red().bold() << "Error: no functions available" << t.normal() << endl;
        return;
    }

    dump_function(mod_idx, module.num_functions() - 1);
}


void ReplCommand::cmd_dump_function(std::string fun_name) {
    TermCtl& t = TermCtl::stdout_instance();
    if (context().modules.empty()) {
        cout << t.red().bold() << "Error: no modules available" << t.normal() << endl;
        return;
    }
    size_t mod_idx = context().modules.size() - 1;
    const auto& module = *context().modules[mod_idx];

    for (size_t i = 0; i != module.num_functions(); ++i) {
        if (module.get_function(i).name() == fun_name) {
            dump_function(mod_idx, i);
            return;
        }
    }
    cout << t.red().bold() << "Error: function not found: " << fun_name
         << t.normal() << endl;
}


void ReplCommand::cmd_dump_function(std::string fun_name, std::string mod_name) {
    TermCtl& t = TermCtl::stdout_instance();

    // lookup module
    size_t mod_idx = 0;
    Module* module = nullptr;
    for (const auto& m : context().modules) {
        if (m->name() == mod_name) {
            module = m.get();
            break;
        }
        ++mod_idx;
    }
    if (mod_idx >= context().modules.size()) {
        cout << t.red().bold() << "Error: module not found: " << mod_name
             << t.normal() << endl;
        return;
    }

    // lookup function
    for (size_t i = 0; i != module->num_functions(); ++i) {
        if (module->get_function(i).name() == fun_name) {
            dump_function(mod_idx, i);
            return;
        }
    }
    cout << t.red().bold() << "Error: function not found: " << fun_name
         << t.normal() << endl;
}


void ReplCommand::cmd_dump_function(size_t fun_idx)
{
    TermCtl& t = TermCtl::stdout_instance();
    if (context().modules.empty()) {
        cout << t.red().bold() << "Error: no modules available" << t.normal() << endl;
        return;
    }
    size_t mod_idx = context().modules.size() - 1;
    dump_function(mod_idx, fun_idx);
}


void ReplCommand::cmd_dump_function(size_t fun_idx, size_t mod_idx)
{
    dump_function(mod_idx, fun_idx);
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
