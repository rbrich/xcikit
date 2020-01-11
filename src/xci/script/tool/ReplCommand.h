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
    // Void
    void add_cmd(std::string&& name, std::string&& alias, Function::NativeWrapper native_fn);
    // Int -> Void
    void add_cmd_i(std::string&& name, std::string&& alias, Function::NativeWrapper native_fn);
    // String -> Void
    void add_cmd_s(std::string&& name, std::string&& alias, Function::NativeWrapper native_fn);

private:
    Interpreter m_interpreter {0};
    Module m_module {"cmd"};
};


}  // namespace xci::script::tool

#endif // include guard
