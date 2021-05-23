// BytecodeTracer.cpp created on 2020-01-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "BytecodeTracer.h"
#include <xci/script/dump.h>

#include <iostream>
#include <string>
#include <stack>

namespace xci::script::tool {

using std::string;
using std::cin;
using std::cout;
using std::endl;
using std::flush;


void BytecodeTracer::setup(bool print_bytecode, bool trace_bytecode)
{
    if (print_bytecode || trace_bytecode) {
        m_machine.set_call_enter_cb([this](const Function& f) {
            auto frame = m_machine.stack().n_frames() - 1;
            print_code(frame, f);
        });
        if (trace_bytecode) {
            m_machine.set_call_exit_cb([this](const Function&) {
                auto frame = m_machine.stack().n_frames() - 1;
                if (frame == 0) {
                    cout << m_term.clear_screen_down();
                    m_lines_to_erase = 0;
                } else {
                    --frame;
                    const auto& f = m_machine.stack().frame().function;
                    print_code(frame, f);
                }
            });
        }
    }

    if (trace_bytecode) {
        m_machine.set_bytecode_trace_cb([this]
                (const Function& f, Code::const_iterator ipos) {
            const auto& t = m_term;
            if (m_lines_to_erase > 0) {
                cout << t.move_up(m_lines_to_erase);
            } else {
                auto frame = m_machine.stack().n_frames() - 1;
                cout << "[" << frame << "] " << f.name() << " " << f.signature() << endl;
            }
            for (auto it = f.code().begin(); it != f.code().end();) {
                if (it == ipos) {
                    cout << t.yellow() << '>' << DumpInstruction{f, it} << t.normal() << endl;
                } else {
                    cout << ' ' << DumpInstruction{f, it} << endl;
                }
            }
            if (ipos == f.code().end()) {
                cout << t.yellow() << "> --- RETURN ---" << t.normal() << endl;
                ++ m_lines_to_erase;
            }
            // pause
            for (;;) {
                cout << "dbg> " << flush;
                string cmd;
                getline(cin, cmd);
                if (cmd == "n" || cmd.empty()) {
                    break;
                } else if (cmd == "s") {
                    cout << "Stack content:" << endl;
                    cout << m_machine.stack() << endl;
                    m_lines_to_erase = 0;
                } else {
                    cout << "Help:\nn    next step\ns    show stack" << endl;
                    m_lines_to_erase = 0;
                }
            }
            if (m_lines_to_erase > 0)
                cout << t.move_up(1);
        });
    }

    if (print_bytecode) {
        m_term.print("Bytecode trace:\n");
    }
}


void BytecodeTracer::print_code(size_t frame, const Function& f)
{
    m_lines_to_erase = 0;
    cout << "[" << frame << "] " << f.name() << " " << f.signature() << endl;
    for (auto it = f.code().begin(); it != f.code().end();) {
        ++ m_lines_to_erase;
        cout << ' ' << DumpInstruction{f, it} << endl;
    }
}


}  // namespace xci::script::tool
