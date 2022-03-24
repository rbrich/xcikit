// Options.cpp.c created on 2021-03-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Options.h"
#include <xci/core/ArgParser.h>
#include <xci/core/string.h>
#include <xci/core/TermCtl.h>

#include <range/v3/all.hpp>
#include <fmt/core.h>

namespace xci::script::tool {

using namespace xci::core;
using namespace xci::core::argparser;

using ranges::views::keys;
using ranges::views::filter;
using ranges::views::transform;
using ranges::accumulate;
using ranges::to;

using Flags = Compiler::Flags;

using PassItem = std::pair<const char*, Flags>;

static PassItem pass_names[] = {
        {"fold_tuple", Flags::PPTuple},
        {"fold_dot_call", Flags::PPDotCall},
        {"resolve_symbols", Flags::PPSymbols},
        {"resolve_types", Flags::PPTypes},
        {"resolve_nonlocals", Flags::PPNonlocals},
};


static std::string output_pass_list()
{
    return fmt::format("{}", fmt::join(pass_names | keys, ","));
}


/// \returns Flags(~0) on error
static Flags pass_name_to_flag(std::string_view name)
{
    auto filtered = pass_names | filter([name](auto&& item) {
        return std::string_view{item.first}.find(name) != std::string_view::npos;
    }) | to< std::vector<PassItem> >();
    auto& t = TermCtl::stderr_instance();
    if (filtered.empty()) {
        t.print("{t:bold}Note:{t:normal} {} did not match any pass name\n", name);
        return Flags(~0);
    }
    if (filtered.size() > 1) {
        t.print("{t:bold}Note:{t:normal} {} matched multiple pass names: {}\n",
                   name, fmt::join(filtered | keys, " "));
        return Flags(~0);
    }
    return filtered[0].second;
}


static bool parse_pass_list(ReplOptions& ro, const char* list_str)
{
    auto items = split(list_str, ',');
    ro.compiler_flags |= accumulate(items | transform(pass_name_to_flag), Flags(0),
               [](Flags a, Flags b) -> Flags { return a | b; });
    return ro.compiler_flags != Flags(~0);
}


void Options::parse(char* argv[])
{
    auto& ro = repl_opts;
    auto& po = prog_opts;
    ArgParser {
            Option("-h, --help", "Show help", show_help),
            Option("-v, --verbose", "Print compilation progress and timing stats", po.verbose),
            Option("-c, --compile", "Compile a module (don't run anything)", po.compile),
            Option("-o, --output FILE", "Output file for compiled module (default is <source basename>.firm)", po.output_file),
            Option("-e, --eval EXPR", "Execute EXPR as main input", po.expr),
            Option("-O, --optimize", "Allow optimizations", [&ro]{ ro.compiler_flags |= Flags::O1; }),
            Option("-r, --raw-ast", "Print raw AST", ro.print_raw_ast),
            Option("-t, --ast", "Print processed AST", ro.print_ast),
            Option("-b, --bytecode", "Print bytecode", ro.print_bytecode),
            Option("-s, --symtab", "Print symbol table", ro.print_symtab),
            Option("-m, --module", "Print compiled module content", ro.print_module),
            Option("-M, --module-verbose", "Print compiled module content, including function bodies", ro.print_module_verbose),
            Option("-T, --module-ast", "Print compiled module content like -M, but dump generic functions as AST", ro.print_module_verbose_ast),
            Option("--trace", "Trace bytecode", ro.trace_bytecode),
            Option("--pp PASS_LIST", "Preprocess AST and stop, don't compile. "
                                     "PASS_LIST is comma separated list of pass names (or unique substrings of them): "
                                     + output_pass_list()
                                     , [&ro](const char* arg){ return parse_pass_list(ro, arg); }),
            Option("-S, --no-std", "Do not import standard library", [&ro]{ ro.with_std_lib = false; }),
            Option("--output-schema FILE", "Write schema of compiled module to FILE (for use in dati tool)", po.schema_file),
            Option("[INPUT ...]", "Input files", [&po](const char* arg)
            { po.input_files.emplace_back(arg); return true; }),
    } (argv);
}


} // namespace xci::script::tool
