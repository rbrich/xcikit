// ReplCommand.cpp created on 2020-01-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ReplCommand.h"
#include "Context.h"
#include <xci/script/Builtin.h>
#include <xci/script/dump.h>
#include <xci/core/string.h>
#include <xci/core/TermCtl.h>

#include <ranges>
#include <iostream>
#include <utility>

namespace xci::script::tool {

using std::ranges::views::reverse;
using std::ranges::views::drop;
using namespace xci::core;


void ReplCommand::cmd_quit() {
    if (m_quit_cb)
        m_quit_cb();
}


static void cmd_help() {
    TermCtl& t = xci::core::TermCtl::stdout_instance();
    t.write(".q, .quit                                  quit\n");
    t.write(".h, .help                                  show all accepted commands\n");
    t.write(".di, .dump_info                            print info about interpreter attributes on this machine\n");
    t.write(".dm, .dump_module [#|NAME]                 print contents of last compiled module (or module by index or by name)\n");
    t.write(".df, .dump_function [#|NAME] [#|MODULE]    print contents of last compiled function (or function by index/name from specified module)\n");
    t.write(".d, .describe NAME                         print information about named symbol (function, type, module)\n");
}


static void cmd_dump_info() {
    TermCtl& t = xci::core::TermCtl::stdout_instance();
    t.write("Bloat:\n");
    t.print("  sizeof(HeapSlot) = {}\n", sizeof(HeapSlot));
    t.print("  sizeof(Value) = {}\n", sizeof(Value));
    t.print("  sizeof(TypeInfo) = {}\n", sizeof(TypeInfo));
    t.print("  sizeof(Function) = {}\n", sizeof(Function));
    t.print("  sizeof(Function::BytecodeBody) = {}\n", sizeof(Function::BytecodeBody));
    t.print("  sizeof(Function::AssemblyBody) = {}\n", sizeof(Function::AssemblyBody));
    t.print("  sizeof(Function::GenericBody) = {}\n", sizeof(Function::GenericBody));
    t.print("  sizeof(Function::NativeBody) = {}\n", sizeof(Function::NativeBody));
}


static void print_module_header(const Module& mod)
{
    TermCtl& t = xci::core::TermCtl::stdout_instance();
    t.stream() << fmt::format("Module \"{}\" ({:x})", mod.name(), intptr_t(&mod.symtab())) << std::endl;
}


const Module* ReplCommand::module_by_idx(Index mod_idx) {
    TermCtl& t = m_ctx.term_out;
    auto& module_manager = m_ctx.interpreter.module_manager();

    if (mod_idx == Index(-1)) {
        auto std_module = module_manager.import_module("std");
        return std_module.get();
    }
    if (mod_idx == Index(-2)) {
        auto builtin_module = module_manager.import_module("builtin");
        return builtin_module.get();
    }
    if (mod_idx == Index(-3))
        return m_module.get();

    if (mod_idx >= m_ctx.input_modules.size()) {
        t.print("<bold><red>Error: module index out of range: {}<normal>\n",
                mod_idx);
        return nullptr;
    }
    return m_ctx.input_modules[mod_idx].get();
}


const Module* ReplCommand::module_by_name(std::string_view mod_name) {
    if (mod_name == ".")
        return m_module.get();

    const auto name_id = intern(mod_name);
    for (const auto& m : m_ctx.input_modules | reverse) {
        if (m->name() == name_id)
            return m.get();
        for (Index i = 0; i != m->num_imported_modules(); ++i) {
            const auto& imp_mod = m->get_imported_module(i);
            if (imp_mod.name() == name_id)
                return &imp_mod;
        }
    }

    TermCtl& t = m_ctx.term_out;
    t.print("<bold><red>Error: module not found: {}<normal>\n",
            mod_name);
    return nullptr;
}


void ReplCommand::dump_module(Index mod_idx) {
    auto* mod = module_by_idx(mod_idx);
    if (!mod)
        return;
    print_module_header(*mod);
    m_ctx.term_out.stream() << *mod << std::endl;
}


void ReplCommand::cmd_dump_module() {
    dump_module(Index(m_ctx.input_modules.size() - 1));
}


void ReplCommand::cmd_dump_module(Index mod_idx) {
    dump_module(mod_idx);
}


void ReplCommand::cmd_dump_module(std::string_view mod_name) {
    auto* mod = module_by_name(mod_name);
    if (!mod)
        return;
    print_module_header(*mod);
    m_ctx.term_out.stream() << *mod << std::endl;
}


void ReplCommand::dump_function(const Module& mod, Index fun_idx) {
    TermCtl& t = m_ctx.term_out;

    if (fun_idx >= mod.num_functions()) {
        t.print("<bold><red>Error: function index out of range: {}<normal>\n",
                fun_idx);
        return;
    }
    const auto& function = mod.get_function(fun_idx);

    print_module_header(mod);
    t.print("Function [{}] {}: ", fun_idx, function.name());
    t.stream() << function << std::endl;
}


void ReplCommand::cmd_dump_function() {
    TermCtl& t = m_ctx.term_out;
    if (m_ctx.input_modules.empty()) {
        t.print("<bold><red>Error: no input modules available<normal>\n");
        return;
    }
    size_t mod_idx = m_ctx.input_modules.size() - 1;
    const auto& mod = *m_ctx.input_modules[mod_idx];

    if (mod.num_functions() == 0) {
        t.print("<bold><red>Error: no functions available<normal>\n");
        return;
    }

    dump_function(mod, mod.num_functions() - 1);
}


void ReplCommand::cmd_dump_function(std::string_view fun_name) {
    TermCtl& t = m_ctx.term_out;
    if (m_ctx.input_modules.empty()) {
        t.print("<bold><red>Error: no input modules available<normal>\n");
        return;
    }
    size_t mod_idx = m_ctx.input_modules.size() - 1;
    const auto& mod = *m_ctx.input_modules[mod_idx];

    const auto fun_name_id = intern(fun_name);
    for (Index i = 0; i != mod.num_functions(); ++i) {
        if (mod.get_function(i).name() == fun_name_id) {
            dump_function(mod, i);
            return;
        }
    }
    t.print("<bold><red>Error: function not found: {}<normal>\n",
            fun_name);
}


void ReplCommand::cmd_dump_function(std::string_view fun_name, std::string_view mod_name) {
    TermCtl& t = m_ctx.term_out;

    // lookup module
    auto* mod = module_by_name(mod_name);
    if (!mod)
        return;

    // lookup function
    const auto fun_name_id = intern(fun_name);
    for (Index i = 0; i != mod->num_functions(); ++i) {
        if (mod->get_function(i).name() == fun_name_id) {
            dump_function(*mod, i);
            return;
        }
    }
    t.print("<bold><red>Error: function not found: {}<normal>\n",
            fun_name);
}


void ReplCommand::cmd_dump_function(Index fun_idx)
{
    TermCtl& t = m_ctx.term_out;
    if (m_ctx.input_modules.empty()) {
        t.print("<bold><red>Error: no modules available<normal>\n");
        return;
    }
    size_t mod_idx = m_ctx.input_modules.size() - 1;
    dump_function(*m_ctx.input_modules[mod_idx], fun_idx);
}


void ReplCommand::cmd_dump_function(Index fun_idx, Index mod_idx)
{
    auto* mod = module_by_idx(mod_idx);
    if (!mod)
        return;
    dump_function(*mod, fun_idx);
}


void ReplCommand::cmd_describe(std::string_view name) {
    TermCtl& t = m_ctx.term_out;

    const auto name_id = intern(name);
    for (const auto& mod : m_ctx.input_modules | reverse) {
        if (mod->name() == name_id) {
            t.print("Module {}: ", name);
            t.stream() << mod->get_main_function().signature() << std::endl;
            return;
        }
        auto sym_ptr = mod->symtab().find_by_name(name_id);
        if (!sym_ptr)
            continue;
        switch (sym_ptr->type()) {
            case Symbol::Module: {
                t.print("Module {} (imported from {}): ", name, mod->name());
                auto& imp_mod = mod->get_imported_module(sym_ptr->index());
                t.stream() << imp_mod.get_main_function().signature() << std::endl;
                return;
            }
            case Symbol::Function: {
                const auto& function = sym_ptr.get_generic_scope().function();
                t.print("Function {}: ", name);
                t.stream() << function.signature() << std::endl;
                return;
            }
            case Symbol::TypeName: {
                const auto& ti = mod->get_type(sym_ptr->index());
                if (ti.is_named() && ti.name() == name_id) {
                    t.print("Named type {} = ", name);
                    t.stream() << ti.underlying() << std::endl;
                } else {
                    t.print("Type alias {} = ", name);
                    t.stream() << ti << std::endl;
                }
                return;
            }
            default:
                t.print("Symbol {} = ", name);
                t.stream() << *sym_ptr << std::endl;
                return;
        }
    }
    t.print("<bold><red>Error: symbol not found: {}<normal>\n",
            name);
}


ReplCommand::ReplCommand(Context& ctx)
        : m_ctx(ctx)
{
    m_module = m_ctx.interpreter.module_manager().make_module("cmd");
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
            [](void* self, std::string_view s)
            { ((ReplCommand*)self)->cmd_dump_module(s); },
            this);
    add_cmd("dump_function", "df",
            [](void* self)
            { ((ReplCommand*)self)->cmd_dump_function(); },
            this);
    add_cmd("dump_function", "df",
            [](void* self, std::string_view f)
            { ((ReplCommand*)self)->cmd_dump_function(f); },
            this);
    add_cmd("dump_function", "df",
            [](void* self, std::string_view f, std::string_view m)
            { ((ReplCommand*)self)->cmd_dump_function(f, m); },
            this);
    add_cmd("dump_function", "df",
            [](void* self, int32_t f)
            { ((ReplCommand*)self)->cmd_dump_function(f); },
            this);
    add_cmd("dump_function", "df",
            [](void* self, int32_t f, int32_t m)
            { ((ReplCommand*)self)->cmd_dump_function(f, m); },
            this);
    add_cmd("describe", "d",
            [](void* self, std::string_view name)
            { ((ReplCommand*)self)->cmd_describe(name); },
            this);
}


void ReplCommand::eval(std::string_view input)
{
    std::string input_str;
    input_str.reserve(input.size());

    auto input_parts = split(input, ' ');
    input_str += input_parts[0];

    // Auto-quote arguments that begin with letters
    for (const auto& part : input_parts | drop(1) ) {
        input_str += ' ';
        const auto first = part.front();
        if (isalpha(first) || first == '_') {
            input_str += '"';
            input_str += part;
            input_str += '"';
        } else {
            input_str += part;
        }
    }

    m_ctx.interpreter.eval(m_module, std::move(input_str));
}


}  // namespace xci::script::tool
