// Repl.h created on 2021-03-16 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TOOL_REPL_H
#define XCI_SCRIPT_TOOL_REPL_H

#include "Context.h"
#include "Options.h"

#include <xci/script/Error.h>
#include <xci/core/Vfs.h>

#include <string>
#include <string_view>

namespace xci::script::tool {

enum class EvalMode {
    Repl,
    SingleInput,
    Compile,
    Preprocess,  // only process and print AST, do not compile
};

class Repl {
public:
    Repl(Context& ctx, const ReplOptions& opts, const core::Vfs & vfs)
        : m_ctx(ctx), m_opts(opts), m_vfs(vfs) {}

    bool evaluate(const std::string& module_name, std::string module_source, EvalMode mode);

    std::unique_ptr<Module> prepare_module(const std::string& module_name);
    bool evaluate_module(Module& module, EvalMode mode);

private:
    void print_error(const ScriptError& e);

    Context& m_ctx;
    const ReplOptions& m_opts;
    const core::Vfs& m_vfs;
};

}  // namespace xci::script::tool

#endif // include guard
