// Options.h created on 2021-03-16 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TOOL_OPTIONS_H
#define XCI_SCRIPT_TOOL_OPTIONS_H

#include <cstdint>
#include <vector>

namespace xci::script::tool {

struct ReplOptions {
    bool print_raw_ast = false;
    bool print_ast = false;
    bool print_symtab = false;
    bool print_module = false;
    bool print_bytecode = false;
    bool trace_bytecode = false;
    bool with_std_lib = true;
    uint32_t compiler_flags = 0;
};

struct ProgramOptions {
    std::vector<const char*> input_files;
    const char* expr = nullptr;
};

struct Options {
    ReplOptions repl_opts;
    ProgramOptions prog_opts;

    void parse(char* argv[]);
};

} // namespace xci::script::tool

#endif // include guard
