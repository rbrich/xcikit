// ReplCommand.h created on 2020-01-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TOOL_REPL_COMMAND_H
#define XCI_SCRIPT_TOOL_REPL_COMMAND_H

#include "Context.h"
#include <xci/script/Interpreter.h>

namespace xci::script::tool {


// Interpret REPL commands, e.q. ".quit"
class ReplCommand {
public:
    ReplCommand(Context& ctx);

    void eval(std::string_view input);

    using Callback = std::function<void()>;
    void set_quit_cb(Callback cb) { m_quit_cb = std::move(cb); }

private:
    template<class F>
    void add_cmd(std::string&& name, std::string&& alias, F&& fun) {
        auto p = m_module->add_native_function(move(name), std::forward<F>(fun));
        m_module->symtab().add(Symbol{move(alias), Symbol::Function, p->index()});
    }
    template<class F>
    void add_cmd(std::string&& name, std::string&& alias, F&& fun, void* arg) {
        auto p = m_module->add_native_function(move(name), std::forward<F>(fun), arg);
        m_module->symtab().add(Symbol{move(alias), Symbol::Function, p->index()});
    }

    void cmd_quit();

    const Module* module_by_idx(size_t mod_idx);
    const Module* module_by_name(std::string_view mod_name);

    void dump_module(size_t mod_idx);
    void cmd_dump_module();
    void cmd_dump_module(size_t mod_idx);
    void cmd_dump_module(std::string_view mod_name);

    void dump_function(const Module& module, size_t fun_idx);
    void cmd_dump_function();
    void cmd_dump_function(std::string_view fun_name);
    void cmd_dump_function(std::string_view fun_name, std::string_view mod_name);
    void cmd_dump_function(size_t fun_idx);
    void cmd_dump_function(size_t fun_idx, size_t mod_idx);

    Context& m_ctx;
    std::shared_ptr<Module> m_module;  // "cmd" module
    Callback m_quit_cb;
};


}  // namespace xci::script::tool

#endif // include guard
