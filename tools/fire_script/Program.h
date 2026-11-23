// Program.h created on 2021-03-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021–2026 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TOOL_PROGRAM_H
#define XCI_SCRIPT_TOOL_PROGRAM_H

#include "Context.h"
#include "Repl.h"
#include "ReplCommand.h"
#include "Options.h"
#include <xci/core/EditLine.h>
#include <xci/vfs/Vfs.h>

#include <string_view>

namespace xci::script::tool {


struct Program {
    Program(bool log_debug = false);
    ~Program();

    /// Parse and evaluate program args
    /// Returns:
    /// * -1 error
    /// *  0 done (--help, --eval etc.)
    /// *  1 continue to REPL
    int process_args(char* argv[]);

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

    Context ctx;
    Options opts;
    Repl repl {ctx, opts.repl_opts};

#ifdef __EMSCRIPTEN__
    // Emscripten needs explicit 'syncfs' after writing to a file
    using Callback = std::function<void()>;
    void set_sync_history_cb(Callback cb) { m_sync_history_cb = std::move(cb); }
    Callback m_sync_history_cb;
#endif
};


} // namespace xci::script::tool

#endif // include guard
