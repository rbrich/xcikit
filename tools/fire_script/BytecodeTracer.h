// BytecodeTracer.h created on 2020-01-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020–2026 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TOOL_BYTECODE_TRACER_H
#define XCI_SCRIPT_TOOL_BYTECODE_TRACER_H

#include <xci/script/Machine.h>
#include <xci/core/TermCtl.h>

namespace xci::script::tool {


class BytecodeTracer {
public:
    explicit BytecodeTracer(Machine& machine, core::TermCtl& term)
        : m_machine(machine), m_term(term) {}

    void setup(bool print_bytecode, bool trace_bytecode);

private:
    void print_code(size_t frame, const Function& f);

    Machine& m_machine;
    core::TermCtl& m_term;
    // lines of code that need to be erased before rendering next step
    unsigned int m_lines_to_erase = 0;
    bool m_prev_was_exit = false;
};


}  // namespace xci::script::tool

#endif // include guard
