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
        // All compiler passes
        // UNSAFE: Don't use these flags directly - the wrong set of flags
        //         can crash the compiler (asserts in Debug mode).
        //         Use PP* flags below.
        FoldTuple           = 0x0001,
        FoldDotCall         = 0x0002,
        ResolveSymbols      = 0x0004,
        ResolveTypes        = 0x0008,
        FoldIntrinsics      = 0x0010,
        ResolveNonlocals    = 0x0020,
        FoldConstExpr       = 0x0001 << 16,

        // Bit masks
        MandatoryMask       = 0xffff,
        OptimizationMask    = 0xffff << 16,

        // Mandatory AST passes
        // - if none of the flags are set, all passes will be enabled
        // - these flags solve passes have internal dependencies, these flags
        PPTuple         = FoldTuple,
        PPDotCall       = FoldDotCall,
        PPSymbols       = ResolveSymbols,
        PPTypes         = ResolveTypes | PPSymbols,
        PPIntrinsics    = FoldIntrinsics | PPTypes,
        PPNonlocals     = ResolveNonlocals,

        // Optimization
        O0 = 0,
        O1 = MandatoryMask | FoldConstExpr,
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
