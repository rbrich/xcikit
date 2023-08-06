// Options.h created on 2021-03-16 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TOOL_OPTIONS_H
#define XCI_SCRIPT_TOOL_OPTIONS_H

#include <xci/script/Compiler.h>
#include <cstdint>
#include <vector>

namespace xci::script::tool {

struct ReplOptions {
    bool print_rusage = false;
    bool print_raw_ast = false;
    bool print_ast = false;
    bool print_symtab = false;
    bool print_module = false;
    bool print_module_verbose = false;
    bool print_module_verbose_ast = false;
    bool print_bytecode = false;
    bool trace_bytecode = false;
    bool with_std_lib = true;
    Compiler::Flags compiler_flags = Compiler::Flags::Mandatory;
    unsigned optimization = 1;

    void apply_optimization();
};

struct ProgramOptions {
    std::vector<const char*> input_files;
    std::string output_file;
    std::string schema_file;
    const char* expr = nullptr;
    bool compile = false;
    bool verbose = false;
};

struct Options {
    ReplOptions repl_opts;
    ProgramOptions prog_opts;

    void parse(char* argv[]);
};

} // namespace xci::script::tool

#endif // include guard
