// cmd_parser.cpp created on 2019-12-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "cmd_parser.h"
#include <xci/script/dump.h>
#include <xci/core/TermCtl.h>
#include <tao/pegtl.hpp>
#include <iostream>

namespace xci::script::repl {

using std::cout;
using std::endl;


namespace cmd_parser {

using namespace xci::core;
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
    static void apply0(Context& ctx) {
        ctx.done = true;
    }
};

template<>
struct Action<Help> {
    static void apply0(Context&) {
        cout << ".q, .quit                      quit" << endl;
        cout << ".h, .help                      show all accepted commands" << endl;
        cout << ".dm, .dump_module [#|name]     print contents of last compiled module (or module by index or by name)" << endl;
    }
};

template<>
struct Action<DumpModule> : change_states< Args > {
    template<typename Input>
    static void success(const Input &in, Args& args, Context& ctx) {
        TermCtl& t = TermCtl::stdout_instance();
        if (ctx.modules.empty()) {
            cout << t.red().bold() << "Error: no modules available" << t.normal() << endl;
            return;
        }

        unsigned mod = 0;
        if (args.num == 1) {
            if (!args.s1.empty()) {
                size_t n = 0;
                for (const auto& m : ctx.modules) {
                    if (m->name() == args.s1) {
                        cout << "Module [" << n << "] " << args.s1 << ":" << endl << *m << endl;
                        return;
                    }
                    ++n;
                }
            } else {
                mod = args.u1;
                if (mod >= ctx.modules.size()) {
                    cout << t.red().bold() << "Error: module index out of range: "
                         << mod << t.normal() << endl;
                    return;
                }
            }
        } else {
            mod = ctx.modules.size() - 1;
        }
        const auto& m = *ctx.modules[mod];
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


void parse_command(const std::string& line, Context& ctx)
{
    using cmd_parser::CmdGrammar;
    using cmd_parser::Action;
    auto& t = ctx.term_out;

    tao::pegtl::memory_input<> in(line, "command");
    try {
        if (!tao::pegtl::parse< CmdGrammar, Action >( in, ctx )) {
            // not matched at all
            cout << t.red().bold() << "Error: unknown command: " << line << " (try .help)" << t.normal() << endl;
        }
    } catch (tao::pegtl::parse_error&) {
        // partially matched, encountered error - possibly wrong argument
        cout << t.red().bold() << "Error: unknown command: " << line << " (try .help)" << t.normal() << endl;
    }
}


}  // namespace xci::script::repl
