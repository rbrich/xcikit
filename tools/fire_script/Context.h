// Context.h created on 2019-12-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TOOL_CONTEXT_H
#define XCI_SCRIPT_TOOL_CONTEXT_H

#include <xci/script/Interpreter.h>
#include <xci/script/Module.h>
#include <xci/script/Function.h>
#include <xci/core/TermCtl.h>
#include <vector>
#include <memory>

namespace xci::script::tool {


// globals, basically
struct Context {
    int input_number = -1;  // -1 = batch mode, 0..N = REPL mode
    core::Vfs vfs;
    Interpreter interpreter {vfs};
    std::vector<std::shared_ptr<xci::script::Module>> input_modules;
    xci::core::TermCtl& term_out = xci::core::TermCtl::stdout_instance();
};


} // namespace xci::script::tool

#endif // include guard
