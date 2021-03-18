// main.cpp created on 2019-05-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020, 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Context.h"
#include "Repl.h"
#include "ReplCommand.h"
#include "Highlighter.h"

#include <xci/script/Error.h>
#include <xci/core/ArgParser.h>
#include <xci/core/TermCtl.h>
#include <xci/core/EditLine.h>
#include <xci/core/file.h>
#include <xci/core/log.h>
#include <xci/core/string.h>
#include <xci/core/sys.h>
#include <xci/config.h>

#include <iostream>
#include <algorithm>

using namespace xci::core;
using namespace xci::core::argparser;
using namespace xci::script;
using namespace xci::script::tool;

using std::cout;
using std::endl;

constexpr const char* version = "0.4";


int main(int argc, char* argv[])
{
    Logger::init(Logger::Level::Warning);

    Options opts;
    std::vector<const char*> input_files;
    const char* expr = nullptr;

    ArgParser {
            Option("-h, --help", "Show help", show_help),
            Option("-e, --eval EXPR", "Execute EXPR as main input", expr),
            Option("-O, --optimize", "Allow optimizations", [&opts]{ opts.compiler_flags |= Compiler::O1; }),
            Option("-r, --raw-ast", "Print raw AST", opts.print_raw_ast),
            Option("-t, --ast", "Print processed AST", opts.print_ast),
            Option("-b, --bytecode", "Print bytecode", opts.print_bytecode),
            Option("-s, --symtab", "Print symbol table", opts.print_symtab),
            Option("-m, --module", "Print compiled module content", opts.print_module),
            Option("--trace", "Trace bytecode", opts.trace_bytecode),
            Option("--pp-tuple", "Stop after fold_tuple pass", [&opts]{ opts.compiler_flags |= Compiler::PPTuple; }),
            Option("--pp-dotcall", "Stop after fold_dot_call pass", [&opts]{ opts.compiler_flags |= Compiler::PPDotCall; }),
            Option("--pp-symbols", "Stop after resolve_symbols pass", [&opts]{ opts.compiler_flags |= Compiler::PPSymbols; }),
            Option("--pp-types", "Stop after resolve_types pass", [&opts]{ opts.compiler_flags |= Compiler::PPTypes; }),
            Option("--pp-nonlocals", "Stop after resolve_nonlocals pass", [&opts]{ opts.compiler_flags |= Compiler::PPNonlocals; }),
            Option("--no-std", "Do not load standard library", [&opts]{ opts.with_std_lib = false; }),
            Option("[INPUT ...]", "Input files", [&input_files](const char* arg)
                { input_files.emplace_back(arg); return true; }),
    } (argv);

    Context ctx;

    Vfs vfs;
    vfs.mount(XCI_SHARE);

    Repl repl(ctx, opts, vfs);

    if (expr) {
        repl.evaluate(expr);
        return 0;
    }

    if (!input_files.empty()) {
        for (const char* input : input_files) {
            auto content = read_text_file(input);
            if (!content) {
                std::cerr << "cannot read file: " << input << std::endl;
                exit(1);
            }
            repl.evaluate(*content);
        }
        return 0;
    }

    TermCtl& t = ctx.term_out;
    EditLine edit_line{EditLine::Multiline};
    auto history_file = xci::core::home_directory_path() / ".xci_fire_history";
    edit_line.open_history_file(history_file);
    edit_line.set_highlight_callback([&t](std::string_view data, unsigned cursor) {
        auto [hl_data, is_open] = Highlighter(t).highlight(data, cursor);
        return EditLine::HighlightResult{hl_data, is_open};
    });

    // standalone interpreter for the control commands
    ReplCommand cmd(ctx);

    repl.print_intro(version);

    int input_number = 0;
    while (!ctx.done) {
        auto [ok, line] = edit_line.input(t.format("{fg:green}_{} ?{t:normal} ", input_number));
        if (!ok) {
            cout << endl;
            break;
        }

        strip(line);
        if (line.empty())
            continue;

        edit_line.add_history(line);

        if (line[0] == '.') {
            // control commands
            try {
                cmd.interpreter().eval(line.substr(1));
            } catch (const ScriptError& e) {
                cout << t.red() << "Error: " << t.bold().red() << e.what() << t.normal() << std::endl;
                if (!e.detail().empty())
                    cout << t.magenta() << e.detail() << t.normal() << std::endl;
                cout << t.yellow() << "Help: .h | .help" << t.normal() << endl;
            }
            continue;
        }

        if (repl.evaluate(line, input_number))
            ++input_number;
    }

    return 0;
}
