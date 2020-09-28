// main.cpp created on 2019-05-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Context.h"
#include "BytecodeTracer.h"
#include "ReplCommand.h"

#include <xci/script/Error.h>
#include <xci/script/Value.h>
#include <xci/script/dump.h>
#include <xci/core/ArgParser.h>
#include <xci/core/TermCtl.h>
#include <xci/core/file.h>
#include <xci/core/Vfs.h>
#include <xci/core/log.h>
#include <xci/core/string.h>
#include <xci/core/sys.h>
#include <xci/config.h>

#include <replxx.hxx>

#include <iostream>
#include <algorithm>
#include <regex>

using namespace xci::core;
using namespace xci::core::argparser;
using namespace xci::script;
using namespace xci::script::tool;
using Replxx = replxx::Replxx;
using namespace std;


struct Options {
    bool print_raw_ast = false;
    bool print_ast = false;
    bool print_symtab = false;
    bool print_module = false;
    bool print_bytecode = false;
    bool trace_bytecode = false;
    bool with_std_lib = true;
    uint32_t compiler_flags = 0;
};


struct Environment {
    Vfs vfs;

    Environment() {
        Logger::init(Logger::Level::Warning);
        vfs.mount(XCI_SHARE);
    }
};

bool evaluate(Environment& env, const string& line, const Options& opts, int input_number=-1)
{
    TermCtl& t = context().term_out;

    auto& parser = context().interpreter.parser();
    auto& compiler = context().interpreter.compiler();
    auto& machine = context().interpreter.machine();

    context().interpreter.configure(opts.compiler_flags);

    try {
        if (opts.with_std_lib && !context().std_module) {
            auto f = env.vfs.read_file("script/std.fire");
            auto content = f.content();
            context().std_module = context().interpreter.build_module("std", content->string_view());
        }

        // parse
        ast::Module ast;
        parser.parse(line, ast);

        if (opts.print_raw_ast) {
            cout << "Raw AST:" << endl << dump_tree << ast << endl;
        }

        // compile
        std::string module_name = input_number >= 0 ? format("input_{}", input_number) : "<input>";
        auto module = std::make_unique<Module>(module_name);
        module->add_imported_module(BuiltinModule::static_instance());
        if (context().std_module)
            module->add_imported_module(*context().std_module);
        for (auto& m : context().input_modules)
            module->add_imported_module(*m);
        auto func_name = input_number == -1 ? "_" : "_" + to_string(input_number);
        auto func = make_unique<Function>(*module, module->symtab());
        compiler.compile(*func, ast);

        // print AST with Compiler modifications
        if (opts.print_ast) {
            cout << "Processed AST:" << endl << dump_tree << ast << endl;
        }

        // print symbol table
        if (opts.print_symtab) {
            cout << "Symbol table:" << endl << module->symtab() << endl;
        }

        // print compiled module content
        if (opts.print_module) {
            cout << "Module content:" << endl << *module << endl;
        }

        // stop if we were only processing the AST, without actual compilation
        if ((opts.compiler_flags & Compiler::PPMask) != 0)
            return false;

        BytecodeTracer tracer(machine, t);
        tracer.setup(opts.print_bytecode, opts.trace_bytecode);

        machine.call(*func, [&](const Value& invoked) {
            if (!invoked.is_void()) {
                cout << t.bold().yellow() << invoked << t.normal() << endl;
            }
        });

        // returned value of last statement
        auto result = machine.stack().pull(func->effective_return_type());
        if (input_number != -1) {
            // REPL mode
            if (!result->is_void()) {
                cout << t.bold().magenta() << func_name << " = "
                     << t.normal()
                     << t.bold() << *result << t.normal() << endl;
            }
            // save result as function `_<N>` in the module
            auto func_idx = module->add_function(move(func));
            module->symtab().add({move(func_name), Symbol::Function, func_idx});

            context().input_modules.push_back(move(module));
        } else {
            // single input mode
            if (!result->is_void()) {
                cout << t.bold() << *result << t.normal() << endl;
            }
        }
        return true;
    } catch (const ScriptError& e) {
        if (!e.file().empty())
            cout << e.file() << ": ";
        cout << t.red().bold() << "Error: " << e.what() << t.normal();
        if (!e.detail().empty())
            cout << endl << t.magenta() << e.detail() << t.normal();
        cout << endl;
        return false;
    }
}


namespace replxx_hook {


using cl = Replxx::Color;
static std::pair<std::regex, cl> regex_color[] {
        // single chars
        {std::regex{"\\("}, cl::WHITE},
        {std::regex{"\\)"}, cl::WHITE},
        {std::regex{"\\["}, cl::WHITE},
        {std::regex{"\\]"}, cl::WHITE},
        {std::regex{"\\{"}, cl::WHITE},
        {std::regex{"\\}"}, cl::WHITE},

        // special variables
        {std::regex{R"(\b_[0-9]+\b)"}, cl::MAGENTA},

        // keywords
        {std::regex{"\\b(if|then|else)\\b"}, cl::BROWN},
        {std::regex{"\\b(true|false)\\b"}, cl::BRIGHTBLUE},
        {std::regex{"\\bfun\\b"}, cl::BRIGHTMAGENTA},

        // commands
        {std::regex{R"(^ *\.h(elp)?\b)"}, cl::YELLOW},
        {std::regex{R"(^ *\.q(uit)?\b)"}, cl::YELLOW},
        {std::regex{R"(^ *\.(dm|dump_module)\b)"}, cl::YELLOW},
        {std::regex{R"(^ *\.(df|dump_function)\b)"}, cl::YELLOW},
        {std::regex{R"(^ *\.(di|dump_info)\b)"}, cl::YELLOW},

        // numbers
        {std::regex{R"(\b[0-9]+\b)"}, cl::BRIGHTCYAN}, // integer
        {std::regex{R"(\b[0-9]*(\.[0-9]|[0-9]\.)[0-9]*\b)"}, cl::CYAN}, // float

        // strings
        {std::regex{"\".*?\""}, cl::BRIGHTGREEN}, // double quotes
        {std::regex{"\'.*?\'"}, cl::GREEN}, // single quotes

        // comments
        {std::regex{"//.*$"}, cl::GRAY},
        {std::regex{R"(/\*.*?\*/)"}, cl::GRAY},
};


void highlighter(std::string const& context, Replxx::colors_t& colors)
{
    // highlight matching regex sequences
    for (auto const& e : regex_color) {
        size_t pos{0};
        std::string str = context;
        std::smatch match;

        while (std::regex_search(str, match, e.first)) {
            std::string c{match[0]};
            std::string prefix(match.prefix().str());
            pos += utf8_length(prefix);
            size_t len(utf8_length(c));

            for (size_t i = 0; i < len; ++i) {
                colors.at(pos + i) = e.second;
            }

            pos += len;
            str = match.suffix();
        }
    }
}

} // namespace replxx_hook


int main(int argc, char* argv[])
{
    Environment env;
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
            Option("--pp-dotcall", "Stop after fold_dot_call pass", [&opts]{ opts.compiler_flags |= Compiler::PPDotCall; }),
            Option("--pp-symbols", "Stop after resolve_symbols pass", [&opts]{ opts.compiler_flags |= Compiler::PPSymbols; }),
            Option("--pp-types", "Stop after resolve_types pass", [&opts]{ opts.compiler_flags |= Compiler::PPTypes; }),
            Option("--pp-nonlocals", "Stop after resolve_nonlocals pass", [&opts]{ opts.compiler_flags |= Compiler::PPNonlocals; }),
            Option("--no-std", "Do not load standard library", [&opts]{ opts.with_std_lib = false; }),
            Option("[INPUT ...]", "Input files", [&input_files](const char* arg)
                { input_files.emplace_back(arg); return true; }),
    } (argv);

    if (expr) {
        evaluate(env, expr, opts);
        return 0;
    }

    if (!input_files.empty()) {
        for (const char* input : input_files) {
            auto content = read_text_file(input);
            if (!content) {
                std::cerr << "cannot read file: " << input << std::endl;
                exit(1);
            }
            evaluate(env, *content, opts);
        }
        return 0;
    }

    TermCtl& t = context().term_out;
    Replxx rx;
    int input_number = 0;
    std::string history_file = xci::core::get_home_dir() + "/.xci_script_history";
    rx.history_load(history_file);
    rx.set_max_history_size(1000);
    rx.set_highlighter_callback(replxx_hook::highlighter);

    // standalone interpreter for the control commands
    ReplCommand cmd;

    cout << t.format("{t:bold}{fg:magenta}🔥 fire script{t:normal} {fg:magenta}v0.3{t:normal}") << endl;
    while (!context().done) {
        const char* input;
        do {
            input = rx.input(t.format("{fg:green}_{} ? {t:normal}", input_number));
        } while (input == nullptr && errno == EAGAIN);

        if (input == nullptr) {
            cout << t.format("{t:bold}{fg:yellow}.quit{t:normal}") << endl;
            break;
        }

        std::string line{input};
        strip(line);
        if (line.empty())
            continue;

        rx.history_add(input);

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

        if (evaluate(env, line, opts, input_number))
            ++input_number;
    }

    rx.history_save(history_file);
    return 0;
}
