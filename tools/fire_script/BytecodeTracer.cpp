// BytecodeTracer.cpp created on 2020-01-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "BytecodeTracer.h"
#include <xci/script/dump.h>

#include <iostream>
#include <string>
#include <stack>

namespace xci::script::tool {

using std::cout;
using std::endl;


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
                    m_term.clear_screen_down().write();
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
            auto& t = m_term;
            if (m_lines_to_erase > 0) {
                t.move_up(m_lines_to_erase).write();
            } else {
                auto frame = m_machine.stack().n_frames() - 1;
                cout << "[" << frame << "] " << f.name() << " " << f.signature() << endl;
            }
            for (auto it = f.bytecode().begin(); it != f.bytecode().end();) {
                if (it == ipos) {
                    cout << t.yellow() << '>' << DumpBytecode{f, it} << t.normal() << endl;
                } else {
                    cout << ' ' << DumpBytecode{f, it} << endl;
                }
            }
            // pause
            for (;;) {
                cout << "dbg> " << std::flush;
                std::string cmd;
                getline(std::cin, cmd);
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
                t.move_up(1).write();
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
    for (auto it = f.bytecode().begin(); it != f.bytecode().end();) {
        ++ m_lines_to_erase;
        cout << ' ' << DumpBytecode{f, it} << endl;
    }
}


}  // namespace xci::script::tool
