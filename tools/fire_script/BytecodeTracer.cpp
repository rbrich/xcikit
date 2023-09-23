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


void BytecodeTracer::setup(bool print_bytecode, bool trace_bytecode)
{
    bool prev_was_exit = false;  // previous cb called was exit
    if (print_bytecode || trace_bytecode) {
        m_machine.set_call_enter_cb([this, &prev_was_exit](const Function& f) {
            const auto frame = m_machine.stack().n_frames() - 1;
            if (prev_was_exit && m_lines_to_erase > 0) {
                // tail call (exit immediately followed by enter)
                m_term.move_up(m_lines_to_erase + 1).write();
                m_term.clear_screen_down().write();
                prev_was_exit = false;
            }
            print_code(frame, f);
        });
        if (trace_bytecode) {
            m_machine.set_call_exit_cb([this, &prev_was_exit](const Function&) {
                auto frame = m_machine.stack().n_frames() - 1;
                if (frame == 0) {
                    m_term.clear_screen_down().write();
                    m_lines_to_erase = 0;
                } else {
                    --frame;
                    const auto& f = m_machine.stack().frame(frame).function;
                    print_code(frame, f);
                }
                prev_was_exit = true;
            });
        }
    }

    if (trace_bytecode) {
        m_machine.set_bytecode_trace_cb([this, &prev_was_exit]
                (const Function& f, Code::const_iterator ipos) {
            prev_was_exit = false;
            auto& t = m_term;
            if (m_lines_to_erase > 0) {
                t.move_up(m_lines_to_erase).write();
            } else {
                auto frame = m_machine.stack().n_frames() - 1;
                fmt::print("[{}] {} {}\n", frame, f.name(), f.signature());
            }
            for (auto it = f.bytecode().begin(); it != f.bytecode().end();) {
                if (it == ipos) {
                    cout << t.yellow() << '>' << DumpBytecode{f, it} << t.normal() << '\n';
                } else {
                    cout << ' ' << DumpBytecode{f, it} << '\n';
                }
            }
            // pause
            for (;;) {
                cout << "dbg> " << std::flush;
                std::string cmd;
                while (!getline(std::cin, cmd))
                    ;
                if (cmd == "n" || cmd.empty()) {
                    break;
                } else if (cmd == "s") {
                    cout << "Stack content:" << '\n';
                    cout << m_machine.stack() << '\n';
                    m_lines_to_erase = 0;
                } else {
                    cout << "Help:\nn    next step\ns    show stack" << '\n';
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
    fmt::print("[{}] {} {}\n", frame, f.name(), f.signature());
    for (auto it = f.bytecode().begin(); it != f.bytecode().end();) {
        ++ m_lines_to_erase;
        cout << ' ' << DumpBytecode{f, it} << '\n';
    }
}


}  // namespace xci::script::tool
