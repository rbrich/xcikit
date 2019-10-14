// demo_script.cpp created on 2019-05-15, part of XCI toolkit
// Copyright 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <xci/script/Interpreter.h>
#include <xci/script/Error.h>
#include <xci/script/Value.h>
#include <xci/script/dump.h>
#include <xci/core/TermCtl.h>
#include <xci/core/file.h>
#include <xci/core/Vfs.h>
#include <xci/core/log.h>
#include <xci/config.h>

#include <docopt.h>
#include <iostream>
#include <vector>
#include <stack>
#include <algorithm>

using namespace std;
using namespace xci::core;
using namespace xci::script;


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


void evaluate(const string& line, const Options& opts)
{
    static TermCtl& t = TermCtl::stdout_instance();
    static Interpreter interpreter;
    static std::vector<std::unique_ptr<Module>> modules;

    auto& parser = interpreter.parser();
    auto& compiler = interpreter.compiler();
    auto& machine = interpreter.machine();

    try {
        if (modules.empty()) {
            interpreter.configure(opts.compiler_flags);
            modules.push_back(std::make_unique<BuiltinModule>());

            if (opts.with_std_lib) {
                auto f = Vfs::default_instance().read_file("script/sys.ys");
                auto content = f.content();
                auto sys_module = interpreter.build_module("sys", content->string_view());
                modules.push_back(move(sys_module));

                /*if (opts.print_module) {
                    cout << "Sys module content:" << endl << compiler.get_module(2) << endl;
                }*/
            }
        }

        // parse
        ast::Module ast;
        parser.parse(line, ast);

        if (opts.print_raw_ast) {
            cout << "Raw AST:" << endl << dump_tree << ast << endl;
        }

        // compile
        auto module = std::make_unique<Module>("<input>");
        for (auto& m : modules)
            module->add_imported_module(*m);
        Function func {*module, module->symtab()};
        compiler.compile(func, ast);

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
            return;

        stack<vector<size_t>> codelines_stack;
        if (opts.print_bytecode || opts.trace_bytecode) {
            machine.set_call_enter_cb([&codelines_stack](const Function& f) {
                cout << "[" << codelines_stack.size() << "] " << f.signature() << endl;
                codelines_stack.emplace();
                for (auto it = f.code().begin(); it != f.code().end(); it++) {
                    codelines_stack.top().push_back(it - f.code().begin());
                    cout << ' ' << f.dump_instruction_at(it) << endl;
                }
            });
            machine.set_call_exit_cb([&codelines_stack, &opts](const Function& function) {
                if (opts.trace_bytecode) {
                    cout
                        << t.move_up(codelines_stack.top().size() + 1)
                        << t.clear_screen_down();
                }
                codelines_stack.pop();
            });
        }
        bool erase = true;
        if (opts.trace_bytecode) {
            machine.set_bytecode_trace_cb([&erase, &codelines_stack, &machine]
            (const Function& f, Code::const_iterator ipos) {
                if (erase)
                    cout << t.move_up(codelines_stack.top().size());
                for (auto it = f.code().begin(); it != f.code().end(); it++) {
                    if (it == ipos) {
                        cout << t.yellow() << '>' << f.dump_instruction_at(it) << t.normal() << endl;
                    } else {
                        cout << ' ' << f.dump_instruction_at(it) << endl;
                    }
                }
                if (ipos == f.code().end()) {
                    cout << "---" << endl;
                    codelines_stack.top().push_back(9999);
                } else {
                    // pause
                    erase = true;
                    for (;;) {
                        cout << "dbg> " << flush;
                        string cmd;
                        getline(cin, cmd);
                        if (cmd == "n" || cmd.empty()) {
                            break;
                        } else if (cmd == "ss") {
                            cout << "Stack content:" << endl;
                            cout << machine.stack() << endl;
                            erase = false;
                        } else {
                            cout << "Help:\nn    next step\nss   show stack" << endl;
                            erase = false;
                        }
                    }
                    if (erase)
                        cout << t.move_up(1);
                }
            });
        }
        machine.call(func, [](const Value& invoked) {
            if (!invoked.is_void()) {
                cout << t.bold().yellow() << invoked << t.normal() << endl;
            }
        });

        // returned value of last statement
        auto result = machine.stack().pull(func.signature().return_type);
        if (!result->is_void()) {
            cout << t.bold() << *result << t.normal() << endl;
        }

        modules.push_back(move(module));
    } catch (const Error& e) {
        if (!e.file().empty())
            cout << e.file() << ": ";
        cout << t.red().bold() << "Error: " << e.what() << t.normal() ;
        if (!e.detail().empty())
            cout << std::endl << t.yellow() << e.detail() << t.normal();
        cout << endl;
    }
}


int main(int argc, char* argv[])
{
    Logger::init(Logger::Level::Warning);
    Vfs::default_instance().mount(XCI_SHARE_DIR);

    map<string, docopt::value> args = docopt::docopt(
            "Usage:\n"
            "    demo_script [options] [INPUT ...]\n"
            "\n"
            "Options:\n"
            "   -e EXPR --eval EXPR    Load EXPR as it was a module, run it and exit\n"
            "   -O --optimize          Allow optimizations\n"
            "   -r --raw-ast           Print raw AST\n"
            "   -t --ast               Print processed AST\n"
            "   -b --bytecode          Print bytecode\n"
            "   -s --symtab            Print symbol table\n"
            "   -m --module            Print compiled module content\n"
            "   --trace                Trace bytecode\n"
            "   --pp-symbols           Stop after symbols pass\n"
            "   --pp-nonlocals         Stop after nonlocals pass\n"
            "   --pp-typecheck         Stop after typecheck pass\n"
            "   --no-std               Do not load standard library\n"
            "   -h --help              Show help\n",
            { argv + 1, argv + argc },
            /*help =*/ true,
            "");

    Options opts;
    opts.print_raw_ast = args["--raw-ast"].asBool();
    opts.print_ast = args["--ast"].asBool();
    opts.print_symtab = args["--symtab"].asBool();
    opts.print_module = args["--module"].asBool();
    opts.print_bytecode = args["--bytecode"].asBool();
    opts.trace_bytecode = args["--trace"].asBool();
    opts.with_std_lib = !args["--no-std"].asBool();

    if (args["--optimize"].asBool())
        opts.compiler_flags |= Compiler::O1;
    if (args["--pp-symbols"].asBool())
        opts.compiler_flags |= Compiler::PPSymbols;
    if (args["--pp-nonlocals"].asBool())
        opts.compiler_flags |= Compiler::PPNonlocals;
    if (args["--pp-typecheck"].asBool())
        opts.compiler_flags |= Compiler::PPTypes;

    if (args["--eval"]) {
        evaluate(args["--eval"].asString(), opts);
        return 0;
    }

    if (!args["INPUT"].asStringList().empty()) {
        for (const auto& input : args["INPUT"].asStringList()) {
            auto content = read_text_file(input);
            if (!content) {
                std::cerr << "cannot read file: " << input << std::endl;
                exit(1);
            }
            evaluate(*content, opts);
        }
        return 0;
    }

    TermCtl& t = TermCtl::stdout_instance();
    string line;
    do {
        cout << t.green() << "λ " << t.normal() << flush;
        // TODO: use replxx for line editing
        if (!getline(cin, line))
            break;
        evaluate(line, opts);
    } while (true);

    return 0;
}
