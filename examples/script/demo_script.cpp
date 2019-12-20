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
#include <xci/core/format.h>
#include <xci/core/string.h>
#include <xci/config.h>

#include <docopt.h>
#include <replxx.hxx>
#include <tao/pegtl.hpp>

#include <iostream>
#include <vector>
#include <stack>
#include <algorithm>
#include <regex>

using namespace xci::core;
using namespace xci::script;
using Replxx = replxx::Replxx;
using std::string;
using std::cin;
using std::cout;
using std::endl;
using std::flush;
using std::stack;
using std::vector;
using std::map;


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


static bool g_done {false};

std::vector<std::unique_ptr<Module>>& modules()
{
    static std::vector<std::unique_ptr<Module>> modules;
    return modules;
}

struct Environment {
    Vfs vfs;

    Environment() {
        Logger::init(Logger::Level::Warning);
        vfs.mount(XCI_SHARE_DIR);
    }
};

bool evaluate(Environment& env, const string& line, const Options& opts, int input_number=-1)
{
    static TermCtl& t = TermCtl::stdout_instance();
    static Interpreter interpreter;

    auto& parser = interpreter.parser();
    auto& compiler = interpreter.compiler();
    auto& machine = interpreter.machine();

    try {
        if (modules().empty()) {
            interpreter.configure(opts.compiler_flags);
            modules().push_back(std::make_unique<BuiltinModule>());

            if (opts.with_std_lib) {
                auto f = env.vfs.read_file("script/sys.ys");
                auto content = f.content();
                auto sys_module = interpreter.build_module("sys", content->string_view());
                modules().push_back(move(sys_module));
            }
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
        for (auto& m : modules())
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
            return false;

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

        // save result as static value `_<N>` in the module
        auto result_idx = module->add_value(std::move(result));
        module->symtab().add({"_" + std::to_string(input_number), result_idx});

        modules().push_back(move(module));
        return true;
    } catch (const Error& e) {
        if (!e.file().empty())
            cout << e.file() << ": ";
        cout << t.red().bold() << "Error: " << e.what() << t.normal() ;
        if (!e.detail().empty())
            cout << std::endl << t.magenta() << e.detail() << t.normal();
        cout << endl;
        return false;
    }
}


namespace cmd_parser {
using namespace tao::pegtl;

// ----------------------------------------------------------------------------
// Grammar

struct Unsigned: seq< plus<digit> > {};

struct Quit: sor<TAO_PEGTL_KEYWORD("q"), TAO_PEGTL_KEYWORD("quit")> {};
struct Help: sor<TAO_PEGTL_KEYWORD("h"), TAO_PEGTL_KEYWORD("help")> {};
struct DumpModule: seq<
            sor<TAO_PEGTL_KEYWORD("dm"), TAO_PEGTL_KEYWORD("dump_module")>,
            star<space>,
            opt<sor< Unsigned, identifier> >
        > {};

struct CmdGrammar: seq<
            one<'.'>,
            sor<Quit, Help, DumpModule>,
            eof
        > {};

// ----------------------------------------------------------------------------
// Actions

struct Args {
    size_t num = 0;
    unsigned u1;
    std::string s1;
};

template<typename Rule>
struct Action : nothing<Rule> {};

template<>
struct Action<Quit> {
    static void apply0() {
        g_done = true;
    }
};

template<>
struct Action<Help> {
    static void apply0() {
        cout << ".q, .quit                      quit" << endl;
        cout << ".h, .help                      show all accepted commands" << endl;
        cout << ".dm, .dump_module [#|name]     print contents of last compiled module (or module by index or by name)" << endl;
    }
};

template<>
struct Action<DumpModule> : change_states< Args > {
    template<typename Input>
    static void success(const Input &in, Args& args) {
        TermCtl& t = TermCtl::stdout_instance();
        if (modules().empty()) {
            cout << t.red().bold() << "Error: no modules available" << t.normal() << endl;
            return;
        }

        unsigned mod = 0;
        if (args.num == 1) {
            if (!args.s1.empty()) {
                size_t n = 0;
                for (const auto& m : modules()) {
                    if (m->name() == args.s1) {
                        cout << "Module [" << n << "] " << args.s1 << ":" << endl << *m << endl;
                        return;
                    }
                    ++n;
                }
            } else {
                mod = args.u1;
                if (mod >= modules().size()) {
                    cout << t.red().bold() << "Error: module index out of range: "
                         << mod << t.normal() << endl;
                    return;
                }
            }
        } else {
            mod = modules().size() - 1;
        }
        const auto& m = *modules()[mod];
        cout << "Module [" << mod << "] " << m.name() << ":" << endl << m << endl;
    }
};

template<>
struct Action<Unsigned> {
    template<typename Input>
    static void apply(const Input &in, Args& args) {
        ++args.num;
        args.u1 = atoi(in.string().c_str());
    }
};

template<>
struct Action<identifier> {
    template<typename Input>
    static void apply(const Input &in, Args& args) {
        ++args.num;
        args.s1 = in.string();
    }
};

// ----------------------------------------------------------------------------

}  // namespace cmd_parser


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
        {std::regex{"^ *\\.h(elp)? *$"}, cl::YELLOW},
        {std::regex{"^ *\\.q(uit)? *$"}, cl::YELLOW},
        {std::regex{"^ *\\.(dm|dump_module) *.*$"}, cl::YELLOW},

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
            int len(utf8_length(c));

            for (int i = 0; i < len; ++i) {
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
        evaluate(env, args["--eval"].asString(), opts);
        return 0;
    }

    if (!args["INPUT"].asStringList().empty()) {
        for (const auto& input : args["INPUT"].asStringList()) {
            auto content = read_text_file(input);
            if (!content) {
                std::cerr << "cannot read file: " << input << std::endl;
                exit(1);
            }
            evaluate(env, *content, opts);
        }
        return 0;
    }

    TermCtl& t = TermCtl::stdout_instance();
    Replxx rx;
    int input_number = 0;
    std::string history_file = "./.xci_script_history";
    rx.history_load(history_file);
    rx.set_max_history_size(1000);
    rx.set_highlighter_callback(replxx_hook::highlighter);

    while (!g_done) {
        const char* input;
        do {
            input = rx.input(t.format("{green}_{}> {normal}", input_number));
        } while (input == nullptr && errno == EAGAIN);

        if (input == nullptr) {
            cout << t.format("{bold}{yellow}.quit{normal}") << endl;
            break;
        }

        std::string line{input};
        strip(line);
        if (line.empty())
            continue;

        rx.history_add(input);

        if (line[0] == '.') {
            // control commands
            using cmd_parser::CmdGrammar;
            using cmd_parser::Action;

            tao::pegtl::memory_input<> in(line, "command");
            try {
                if (!tao::pegtl::parse< CmdGrammar, Action >( in )) {
                    // not matched at all
                    cout << t.red().bold() << "Error: unknown command: " << line << " (try .help)" << t.normal() << endl;
                }
            } catch (tao::pegtl::parse_error&) {
                // partially matched, encountered error - possibly wrong argument
                cout << t.red().bold() << "Error: unknown command: " << line << " (try .help)" << t.normal() << endl;
            }
            continue;
        }

        if (evaluate(env, line, opts, input_number))
            ++input_number;
    }

    rx.history_save(history_file);
    return 0;
}
