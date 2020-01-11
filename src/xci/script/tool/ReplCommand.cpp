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

namespace xci::script::tool {

using namespace xci::core;
using std::cout;
using std::endl;


static void cmd_quit(xci::script::Stack& stack) {
    stack.push(value::Void{});  // return void
    context().done = true;
}


static void cmd_help(xci::script::Stack& stack) {
    stack.push(value::Void{});  // return void
    cout << ".q, .quit                                  quit" << endl;
    cout << ".h, .help                                  show all accepted commands" << endl;
    cout << ".dm, .dump_module [#|name]                 print contents of last compiled module (or module by index or by name)" << endl;
    cout << ".df, .dump_function [#|name] [#|module]    print contents of last compiled function (or function by index/name from specified module)" << endl;
}


static void dump_module(unsigned mod_idx) {
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

static void cmd_dump_module(xci::script::Stack& stack) {
    stack.push(value::Void{});  // return void
    dump_module(context().modules.size() - 1);
}


static void cmd_dump_module_i(xci::script::Stack& stack) {
    auto mod_idx = stack.pull<value::Int32>().value();  // 1st arg
    stack.push(value::Void{});  // return void
    dump_module(mod_idx);
}


static void cmd_dump_module_s(xci::script::Stack& stack) {
    auto mod_name = stack.pull<value::String>().value();  // 1st arg
    stack.push(value::Void{});  // return void

    size_t n = 0;
    for (const auto& m : context().modules) {
        if (m->name() == mod_name) {
            dump_module(n);
            return;
        }
        ++n;
    }

    TermCtl& t = TermCtl::stdout_instance();
    cout << t.red().bold() << "Error: module not found: " << mod_name
         << t.normal() << endl;
}


ReplCommand::ReplCommand()
{
    m_interpreter.add_imported_module(m_module);
    add_cmd("quit", "q", cmd_quit);
    add_cmd("help", "h", cmd_help);
    add_cmd("dump_module", "dm", cmd_dump_module);
    add_cmd_i("dump_module", "dm", cmd_dump_module_i);
    add_cmd_s("dump_module", "dm", cmd_dump_module_s);
    m_module.symtab().detect_overloads("dump_module");
    m_module.symtab().detect_overloads("dm");
}


void ReplCommand::add_cmd(std::string&& name, std::string&& alias,
        Function::NativeWrapper native_fn)
{
    auto index = m_module.add_native_function(move(name),
            {}, TypeInfo{Type::Void},
            native_fn);
    m_module.symtab().add({move(alias), Symbol::Function, index});
}


void ReplCommand::add_cmd_i(std::string&& name, std::string&& alias,
        Function::NativeWrapper native_fn)
{
    auto index = m_module.add_native_function(move(name),
            {TypeInfo{Type::Int32}}, TypeInfo{Type::Void},
            native_fn);
    m_module.symtab().add({move(alias), Symbol::Function, index});
}


void ReplCommand::add_cmd_s(std::string&& name, std::string&& alias,
        Function::NativeWrapper native_fn)
{
    auto index = m_module.add_native_function(move(name),
            {TypeInfo{Type::String}}, TypeInfo{Type::Void},
            native_fn);
    m_module.symtab().add({move(alias), Symbol::Function, index});
}


}  // namespace xci::script::tool
