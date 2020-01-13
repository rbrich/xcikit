// ReplCommand.h created on 2020-01-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TOOL_REPL_COMMAND_H
#define XCI_SCRIPT_TOOL_REPL_COMMAND_H

#include <xci/script/Interpreter.h>

namespace xci::script::tool {


// Interpret REPL commands, e.q. ".quit"
class ReplCommand {
public:
    ReplCommand();

    Interpreter& interpreter() { return m_interpreter; }

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

    void dump_module(unsigned mod_idx);
    void cmd_dump_module();
    void cmd_dump_module(int32_t mod_idx);
    void cmd_dump_module(std::string mod_name);

    void dump_function(unsigned mod_idx, unsigned fun_idx);
    void cmd_dump_function();
    void cmd_dump_function(std::string fun_name);
    void cmd_dump_function(std::string fun_name, std::string mod_name);
    void cmd_dump_function(int32_t fun_idx);
    void cmd_dump_function(int32_t fun_idx, int32_t mod_idx);

private:
    Interpreter m_interpreter;
    Module m_module {"cmd"};
};


}  // namespace xci::script::tool

#endif // include guard
