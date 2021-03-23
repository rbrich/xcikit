// Program.h created on 2021-03-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TOOL_PROGRAM_H
#define XCI_SCRIPT_TOOL_PROGRAM_H

#include "Context.h"
#include "Repl.h"
#include "ReplCommand.h"
#include "Options.h"
#include <xci/core/EditLine.h>
#include <xci/core/Vfs.h>

#include <string_view>

namespace xci::script::tool {


struct Program {
    Program(bool log_debug = false);

    /// Parse and evaluate program args
    /// Exits if done (--help, --eval etc.), returns control otherwise
    /// (continue to REPL in that case)
    void process_args(char* argv[]);

    /// Initialize line editing, print intro message
    void repl_init();

    /// REPL - main loop (blocks until user quit)
    /// Alternatively, use `repl_prompt` and `repl_step`.
    void repl_loop();

    /// Call this to show the initial prompt
    void repl_prompt();
    /// Feed input data as they are ready
    void repl_step(std::string_view partial_input);

    void evaluate_input(std::string_view input);

    // standalone interpreter for the control commands
    ReplCommand& repl_command();

    // Line editing widget
    core::EditLine& edit_line();

    core::Vfs vfs;
    Context ctx;
    Options opts;
    Repl repl {ctx, opts.repl_opts, vfs};
};


} // namespace xci::script::tool

#endif // include guard
