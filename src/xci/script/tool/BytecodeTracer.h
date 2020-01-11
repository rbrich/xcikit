// BytecodeTracer.h created on 2020-01-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCIKIT_BYTECODETRACER_H
#define XCIKIT_BYTECODETRACER_H

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

private:
    Machine& m_machine;
    core::TermCtl& m_term;
    // lines of code that need to be erased before rendering next step
    size_t m_lines_to_erase = 0;
};


}  // namespace xci::script::tool

#endif // include guard
