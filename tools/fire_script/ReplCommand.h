// ReplCommand.h created on 2020-01-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
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

    Interpreter& interpreter() { return m_interpreter; }

    using Callback = std::function<void()>;
    void set_quit_cb(Callback cb) { m_quit_cb = std::move(cb); }

private:
    template<class F>
    void add_cmd(std::string&& name, std::string&& alias, F&& fun) {
        auto index = m_module.add_native_function(move(name), std::forward<F>(fun));
        m_module.symtab().add({move(alias), Symbol::Function, index});
    }
    template<class F>
    void add_cmd(std::string&& name, std::string&& alias, F&& fun, void* arg) {
        auto index = m_module.add_native_function(move(name), std::forward<F>(fun), arg);
        m_module.symtab().add({move(alias), Symbol::Function, index});
    }

    void cmd_quit();

    const Module* module_by_idx(size_t mod_idx);
    const Module* module_by_name(const std::string& mod_name);

    void dump_module(size_t mod_idx);
    void cmd_dump_module();
    void cmd_dump_module(size_t mod_idx);
    void cmd_dump_module(std::string mod_name);

    void dump_function(const Module& module, size_t fun_idx);
    void cmd_dump_function();
    void cmd_dump_function(std::string fun_name);
    void cmd_dump_function(std::string fun_name, std::string mod_name);
    void cmd_dump_function(size_t fun_idx);
    void cmd_dump_function(size_t fun_idx, size_t mod_idx);

    Context& m_ctx;
    Interpreter m_interpreter;  // second interpreter, just for the commands
    Module m_module {"cmd"};
    Callback m_quit_cb;
};


}  // namespace xci::script::tool

#endif // include guard
