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


void ReplCommand::cmd_quit() {
    if (m_quit_cb)
        m_quit_cb();
}


static void cmd_help() {
    TermCtl& t = xci::core::TermCtl::stdout_instance();
    t.write(".q, .quit                                  quit\n");
    t.write(".h, .help                                  show all accepted commands\n");
    t.write(".dm, .dump_module [#|name]                 print contents of last compiled module (or module by index or by name)\n");
    t.write(".df, .dump_function [#|name] [#|module]    print contents of last compiled function (or function by index/name from specified module)\n");
    t.write(".di, .dump_info                            print info about interpreter attributes on this machine\n");
}


static void cmd_dump_info() {
    TermCtl& t = xci::core::TermCtl::stdout_instance();
    t.write("Bloat:\n");
    t.print("  sizeof(Function) = {}\n", sizeof(Function));
    t.print("  sizeof(Function::CompiledBody) = {}\n", sizeof(Function::CompiledBody));
    t.print("  sizeof(Function::GenericBody) = {}\n", sizeof(Function::GenericBody));
    t.print("  sizeof(Function::NativeBody) = {}\n", sizeof(Function::NativeBody));
}


static void print_module_header(const Module& module)
{
    TermCtl& t = xci::core::TermCtl::stdout_instance();
    t.stream() << "Module \"" << module.name() << '"'
         << " (" << std::hex << intptr_t(&module.symtab()) << std::dec << ')' << std::endl;
}


const Module* ReplCommand::module_by_idx(size_t mod_idx) {
    TermCtl& t = m_ctx.term_out;

    if (mod_idx == size_t(-1)) {
        if (!m_ctx.std_module) {
            t.print("{t:bold}{fg:red}Error: std module not loaded{t:normal}\n");
            return nullptr;
        }
        return m_ctx.std_module.get();
    }
    if (mod_idx == size_t(-2))
        return &BuiltinModule::static_instance();

    if (mod_idx == size_t(-3))
        return &m_module;

    if (mod_idx >= m_ctx.input_modules.size()) {
        t.print("{t:bold}{fg:red}Error: module index out of range: {}{t:normal}\n",
                mod_idx);
        return nullptr;
    }
    return m_ctx.input_modules[mod_idx].get();
}


const Module* ReplCommand::module_by_name(const std::string& mod_name) {
    size_t n = 0;
    for (const auto& m : m_ctx.input_modules) {
        if (m->name() == mod_name)
            return m.get();
        ++n;
    }

    auto& ctx = m_ctx;
    TermCtl& t = m_ctx.term_out;

    if (mod_name == "std") {
        if (!ctx.std_module) {
            t.print("{t:bold}{fg:red}Error: std module not loaded{t:normal}\n");
            return nullptr;
        }
        return ctx.std_module.get();
    }

    if (mod_name == "builtin")
        return &BuiltinModule::static_instance();

    if (mod_name == "." || mod_name == "cmd")
        return &m_module;

    t.print("{t:bold}{fg:red}Error: module not found: {}{t:normal}\n",
            mod_name);
    return nullptr;
}


void ReplCommand::dump_module(size_t mod_idx) {
    auto* module = module_by_idx(mod_idx);
    if (!module)
        return;
    print_module_header(*module);
    m_ctx.term_out.stream() << *module << std::endl;
}


void ReplCommand::cmd_dump_module() {
    dump_module(m_ctx.input_modules.size() - 1);
}


void ReplCommand::cmd_dump_module(size_t mod_idx) {
    dump_module(mod_idx);
}


void ReplCommand::cmd_dump_module(std::string mod_name) {
    auto* module = module_by_name(mod_name);
    if (!module)
        return;
    print_module_header(*module);
    m_ctx.term_out.stream() << *module << std::endl;
}


void ReplCommand::dump_function(const Module& module, size_t fun_idx) {
    TermCtl& t = m_ctx.term_out;

    if (fun_idx >= module.num_functions()) {
        t.print("{t:bold}{fg:red}Error: function index out of range: {}{t:normal}\n",
                fun_idx);
        return;
    }
    const auto& function = module.get_function(fun_idx);

    print_module_header(module);
    t.print("Function [{}] {}: ", fun_idx, function.name());
    t.stream() << function << std::endl;
}


void ReplCommand::cmd_dump_function() {
    TermCtl& t = m_ctx.term_out;
    if (m_ctx.input_modules.empty()) {
        t.print("{t:bold}{fg:red}Error: no input modules available{t:normal}\n");
        return;
    }
    size_t mod_idx = m_ctx.input_modules.size() - 1;
    const auto& module = *m_ctx.input_modules[mod_idx];

    if (module.num_functions() == 0) {
        t.print("{t:bold}{fg:red}Error: no functions available{t:normal}\n");
        return;
    }

    dump_function(module, module.num_functions() - 1);
}


void ReplCommand::cmd_dump_function(std::string fun_name) {
    TermCtl& t = m_ctx.term_out;
    if (m_ctx.input_modules.empty()) {
        t.print("{t:bold}{fg:red}Error: no input modules available{t:normal}\n");
        return;
    }
    size_t mod_idx = m_ctx.input_modules.size() - 1;
    const auto& module = *m_ctx.input_modules[mod_idx];

    for (size_t i = 0; i != module.num_functions(); ++i) {
        if (module.get_function(i).name() == fun_name) {
            dump_function(module, i);
            return;
        }
    }
    t.print("{t:bold}{fg:red}Error: function not found: {}{t:normal}\n",
            fun_name);
}


void ReplCommand::cmd_dump_function(std::string fun_name, std::string mod_name) {
    TermCtl& t = m_ctx.term_out;

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
    t.print("{t:bold}{fg:red}Error: function not found: {}{t:normal}\n",
            fun_name);
}


void ReplCommand::cmd_dump_function(size_t fun_idx)
{
    TermCtl& t = m_ctx.term_out;
    if (m_ctx.input_modules.empty()) {
        t.print("{t:bold}{fg:red}Error: no modules available{t:normal}\n");
        return;
    }
    size_t mod_idx = m_ctx.input_modules.size() - 1;
    dump_function(*m_ctx.input_modules[mod_idx], fun_idx);
}


void ReplCommand::cmd_dump_function(size_t fun_idx, size_t mod_idx)
{
    auto* module = module_by_idx(mod_idx);
    if (!module)
        return;
    dump_function(*module, fun_idx);
}


ReplCommand::ReplCommand(Context& ctx) : m_ctx(ctx)
{
    m_interpreter.add_imported_module(m_module);
    add_cmd("quit", "q", [](void* self) { ((ReplCommand*)self)->cmd_quit(); }, this);
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
