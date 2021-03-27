// Options.cpp.c created on 2021-03-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Options.h"
#include <xci/core/ArgParser.h>
#include <xci/script/Compiler.h>

namespace xci::script::tool {

using namespace xci::core::argparser;


void Options::parse(char* argv[])
{
    auto& ro = repl_opts;
    auto& po = prog_opts;
    ArgParser {
            Option("-h, --help", "Show help", show_help),
            Option("-e, --eval EXPR", "Execute EXPR as main input", po.expr),
            Option("-O, --optimize", "Allow optimizations", [&ro]{ ro.compiler_flags |= Compiler::O1; }),
            Option("-r, --raw-ast", "Print raw AST", ro.print_raw_ast),
            Option("-t, --ast", "Print processed AST", ro.print_ast),
            Option("-b, --bytecode", "Print bytecode", ro.print_bytecode),
            Option("-s, --symtab", "Print symbol table", ro.print_symtab),
            Option("-m, --module", "Print compiled module content", ro.print_module),
            Option("--trace", "Trace bytecode", ro.trace_bytecode),
            Option("--pp-tuple", "Stop after fold_tuple pass", [&ro]{ ro.compiler_flags |= Compiler::PPTuple; }),
            Option("--pp-dotcall", "Stop after fold_dot_call pass", [&ro]{ ro.compiler_flags |= Compiler::PPDotCall; }),
            Option("--pp-symbols", "Stop after resolve_symbols pass", [&ro]{ ro.compiler_flags |= Compiler::PPSymbols; }),
            Option("--pp-types", "Stop after resolve_types pass", [&ro]{ ro.compiler_flags |= Compiler::PPTypes; }),
            Option("--pp-nonlocals", "Stop after resolve_nonlocals pass", [&ro]{ ro.compiler_flags |= Compiler::PPNonlocals; }),
            Option("--no-std", "Do not load standard library", [&ro]{ ro.with_std_lib = false; }),
            Option("[INPUT ...]", "Input files", [&po](const char* arg)
            { po.input_files.emplace_back(arg); return true; }),
    } (argv);
}


} // namespace xci::script::tool
