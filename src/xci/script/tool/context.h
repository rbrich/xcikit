// context.h created on 2019-12-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_REPL_CONTEXT_H
#define XCI_SCRIPT_REPL_CONTEXT_H

#include <xci/script/Module.h>
#include <xci/script/Function.h>
#include <xci/core/TermCtl.h>
#include <vector>
#include <memory>

namespace xci::script::tool {


struct Context {
    bool done {false};
    std::vector<std::unique_ptr<xci::script::Module>> modules;
    xci::core::TermCtl term_out = xci::core::TermCtl::stdout_instance();
};


} // namespace xci::script::repl

#endif // include guard
