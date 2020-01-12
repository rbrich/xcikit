// ReplCommand.cpp created on 2020-01-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ReplCommand.h"
#include "Context.h"
#include <xci/script/dump.h>
#include <xci/core/TermCtl.h>
#include <iostream>
#include <utility>

namespace xci::script::tool {

using namespace xci::core;
using std::cout;
using std::endl;


static void cmd_quit() {
    context().done = true;
}


static void cmd_help() {
    cout << ".q, .quit                                  quit" << endl;
    cout << ".h, .help                                  show all accepted commands" << endl;
    cout << ".dm, .dump_module [#|name]                 print contents of last compiled module (or module by index or by name)" << endl;
    cout << ".df, .dump_function [#|name] [#|module]    print contents of last compiled function (or function by index/name from specified module)" << endl;
}


void ReplCommand::dump_module(unsigned mod_idx) {
    // dump command module itself
    if (mod_idx == unsigned(-1)) {
        cout << "Command module:" << endl << m_module << endl;
        return;
    }

    auto& ctx = context();
    TermCtl& t = TermCtl::stdout_instance();
    if (ctx.modules.empty()) {
        cout << t.red().bold() << "Error: no modules available" << t.normal() << endl;
        return;
    }
    if (mod_idx >= ctx.modules.size()) {
        cout << t.red().bold() << "Error: module index out of range: "
             << mod_idx << t.normal() << endl;
        return;
    }

    const auto& module = *ctx.modules[mod_idx];
    cout << "Module [" << mod_idx << "] " << module.name() << ":" << endl
         << module << endl;
}


void ReplCommand::cmd_dump_module() {
    dump_module(context().modules.size() - 1);
}


void ReplCommand::cmd_dump_module(int32_t mod_idx) {
    dump_module(mod_idx);
}


void ReplCommand::cmd_dump_module(std::string mod_name) {
    size_t n = 0;
    for (const auto& m : context().modules) {
        if (m->name() == mod_name) {
            dump_module(n);
            return;
        }
        ++n;
    }

    // dump command module itself
    if (mod_name == "." || mod_name == "cmd") {
        dump_module(-1);
        return;
    }

    TermCtl& t = TermCtl::stdout_instance();
    cout << t.red().bold() << "Error: module not found: " << mod_name
         << t.normal() << endl;
}


ReplCommand::ReplCommand()
{
    m_interpreter.add_imported_module(m_module);
    add_cmd("quit", "q", cmd_quit);
    add_cmd("help", "h", cmd_help);
    add_cmd("dump_module", "dm", [](void* self){ ((ReplCommand*)self)->cmd_dump_module(); }, this);
    add_cmd("dump_module", "dm", [](void* self, int32_t i){ ((ReplCommand*)self)->cmd_dump_module(i); }, this);
    add_cmd("dump_module", "dm", [](void* self, std::string s){ ((ReplCommand*)self)->cmd_dump_module(std::move(s)); }, this);
    m_module.symtab().detect_overloads("dump_module");
    m_module.symtab().detect_overloads("dm");
}


}  // namespace xci::script::tool
