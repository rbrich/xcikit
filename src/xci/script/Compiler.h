// Compiler.h created on 2019-05-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_COMPILER_H
#define XCI_SCRIPT_COMPILER_H

#include "ast/AST.h"
#include "Function.h"
#include "Module.h"
#include <vector>

namespace xci::script {


class Compiler {
public:
    enum Flags {
        // enable optimizations
        OConstFold = 0x1,
        O0 = 0,
        O1 = OConstFold,

        // parse & post-process only, do not compile into bytecode
        PPMask          = 7 << 24,
        PPTuple         = 1 << 24,    // enable fold_tuple pass
        PPDotCall       = 2 << 24,    // enable fold_dot_call pass
        PPSymbols       = 3 << 24,    // enable resolve_symbols pass
        PPTypes         = 4 << 24,    // enable resolve_types pass
        PPNonlocals     = 5 << 24,    // enable resolve_nonlocals pass
    };

    Compiler() = default;
    explicit Compiler(uint32_t flags) : m_flags(flags) {}

    void set_flags(uint32_t flags) { m_flags = flags; }
    uint32_t flags() const { return m_flags; }

    // Compile AST into Function object, which contains objects in scope + code
    // (module is special kind of function, with predefined parameters)
    void compile(Function& func, ast::Module& ast);

    // Compile block, return type of its return value
    void compile_block(Function& func, const ast::Block& block);

private:
    uint32_t m_flags = 0;
};


} // namespace xci::script

#endif // include guard
