// ReplCommand.cpp created on 2020-01-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ReplCommand.h"
#include "Context.h"
#include <range/v3/view/reverse.hpp>
#include <range/v3/view/drop.hpp>
#include <xci/script/Builtin.h>
#include <xci/script/dump.h>
#include <xci/core/string.h>
#include <xci/core/TermCtl.h>
#include <iostream>
#include <utility>

namespace xci::script::tool {

using ranges::cpp20::views::reverse;
using ranges::cpp20::views::drop;
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
        t.print("{t:bold}{fg:red}Error: module index out of range: {}{t:normal}\n",
                mod_idx);
        return nullptr;
    }
    return m_ctx.input_modules[mod_idx].get();
}


const Module* ReplCommand::module_by_name(std::string_view mod_name) {
    if (mod_name == ".")
        return m_module.get();

    for (const auto& m : m_ctx.input_modules | reverse) {
        if (m->name() == mod_name)
            return m.get();
        for (Index i = 0; i != m->num_imported_modules(); ++i) {
            const auto& imp_mod = m->get_imported_module(i);
            if (imp_mod.name() == mod_name)
                return &imp_mod;
        }
    }

    TermCtl& t = m_ctx.term_out;
    t.print("{t:bold}{fg:red}Error: module not found: {}{t:normal}\n",
            mod_name);
    return nullptr;
}


void ReplCommand::dump_module(Index mod_idx) {
    auto* module = module_by_idx(mod_idx);
    if (!module)
        return;
    print_module_header(*module);
    m_ctx.term_out.stream() << *module << std::endl;
}


void ReplCommand::cmd_dump_module() {
    dump_module(Index(m_ctx.input_modules.size() - 1));
}


void ReplCommand::cmd_dump_module(Index mod_idx) {
    dump_module(mod_idx);
}


void ReplCommand::cmd_dump_module(std::string_view mod_name) {
    auto* module = module_by_name(mod_name);
    if (!module)
        return;
    print_module_header(*module);
    m_ctx.term_out.stream() << *module << std::endl;
}


void ReplCommand::dump_function(const Module& module, Index fun_idx) {
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


void ReplCommand::cmd_dump_function(std::string_view fun_name) {
    TermCtl& t = m_ctx.term_out;
    if (m_ctx.input_modules.empty()) {
        t.print("{t:bold}{fg:red}Error: no input modules available{t:normal}\n");
        return;
    }
    size_t mod_idx = m_ctx.input_modules.size() - 1;
    const auto& module = *m_ctx.input_modules[mod_idx];

    for (Index i = 0; i != module.num_functions(); ++i) {
        if (module.get_function(i).name() == fun_name) {
            dump_function(module, i);
            return;
        }
    }
    t.print("{t:bold}{fg:red}Error: function not found: {}{t:normal}\n",
            fun_name);
}


void ReplCommand::cmd_dump_function(std::string_view fun_name, std::string_view mod_name) {
    TermCtl& t = m_ctx.term_out;

    // lookup module
    auto* module = module_by_name(mod_name);
    if (!module)
        return;

    // lookup function
    for (Index i = 0; i != module->num_functions(); ++i) {
        if (module->get_function(i).name() == fun_name) {
            dump_function(*module, i);
            return;
        }
    }
    t.print("{t:bold}{fg:red}Error: function not found: {}{t:normal}\n",
            fun_name);
}


void ReplCommand::cmd_dump_function(Index fun_idx)
{
    TermCtl& t = m_ctx.term_out;
    if (m_ctx.input_modules.empty()) {
        t.print("{t:bold}{fg:red}Error: no modules available{t:normal}\n");
        return;
    }
    size_t mod_idx = m_ctx.input_modules.size() - 1;
    dump_function(*m_ctx.input_modules[mod_idx], fun_idx);
}


void ReplCommand::cmd_dump_function(Index fun_idx, Index mod_idx)
{
    auto* module = module_by_idx(mod_idx);
    if (!module)
        return;
    dump_function(*module, fun_idx);
}


void ReplCommand::cmd_describe(std::string_view name) {
    TermCtl& t = m_ctx.term_out;

    for (const auto& module : m_ctx.input_modules | reverse) {
        if (module->name() == name) {
            t.print("Module {}\n", name);
            return;
        }
        auto sym_ptr = module->symtab().find_by_name(name);
        if (!sym_ptr) {
            // FIXME: this is workaround, remove when all imported modules have a relevant symbol
            for (Index i = 0; i != module->num_imported_modules(); ++i) {
                const auto& imp_mod = module->get_imported_module(i);
                if (imp_mod.name() == name) {
                    t.print("Module {} (imported from {})\n",
                            name, module->name());
                    return;
                }
            }
            continue;
        }
        switch (sym_ptr->type()) {
            case Symbol::Module:
                t.print("Module {} (imported from {})\n",
                        name, module->name());
                return;
            case Symbol::Function: {
                const auto& function = module->get_function(sym_ptr->index());
                t.print("Function {}: ", name);
                t.stream() << function.signature() << std::endl;
                return;
            }
            case Symbol::TypeName: {
                const auto& ti = module->get_type(sym_ptr->index());
                if (ti.is_named() && ti.name() == name) {
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
    t.print("{t:bold}{fg:red}Error: symbol not found: {}{t:normal}\n",
            name);
}


ReplCommand::ReplCommand(Context& ctx)
        : m_ctx(ctx),
          m_module(std::make_shared<Module>(m_ctx.interpreter.module_manager()))
{
    m_ctx.interpreter.add_module("cmd", m_module);
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
    m_module->symtab().detect_overloads("dump_module");
    m_module->symtab().detect_overloads("dm");
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
    m_module->symtab().detect_overloads("dump_function");
    m_module->symtab().detect_overloads("df");
    add_cmd("describe", "d",
            [](void* self, std::string_view name)
            { ((ReplCommand*)self)->cmd_describe(name); },
            this);
}


void ReplCommand::eval(std::string_view input)
{
    Module module(m_ctx.interpreter.module_manager());
    module.add_imported_module(m_module);

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

    m_ctx.interpreter.eval(module, std::move(input_str));
}


}  // namespace xci::script::tool
