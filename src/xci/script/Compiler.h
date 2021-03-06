// Compiler.h created on 2019-05-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
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
    enum class Flags {
        // All compiler passes
        // UNSAFE: Don't use these flags directly - the wrong set of flags
        //         can crash the compiler (asserts in Debug mode).
        //         Use PP* flags below.
        FoldTuple           = 0x0001,
        FoldDotCall         = 0x0002,
        ResolveSymbols      = 0x0004,
        ResolveTypes        = 0x0008,
        ResolveNonlocals    = 0x0020,
        FoldConstExpr       = 0x0001 << 16,

        // Bit masks
        MandatoryMask       = 0xffff,
        OptimizationMask    = 0xffff << 16,

        // Mandatory AST passes
        // - if none of the flags are set, all passes will be enabled
        // - if one or more of these flags is set, the Compiler won't compile, only preprocess
        // - each flag may bring in other flags as its dependencies
        PPTuple         = FoldTuple,
        PPDotCall       = FoldDotCall,
        PPSymbols       = ResolveSymbols,
        PPTypes         = ResolveTypes | PPSymbols,
        PPNonlocals     = ResolveNonlocals | PPTypes,

        // Optimization
        O0 = 0,
        O1 = MandatoryMask | FoldConstExpr,

        Default = 0,
    };

    // Allow basic arithmetic on OpCode
    friend inline Flags operator|(Flags a, Flags b) { return Flags(int32_t(a) | int32_t(b)); }
    friend inline Flags operator&(Flags a, Flags b) { return Flags(int32_t(a) & int32_t(b)); }
    friend inline Flags operator|=(Flags& a, Flags b) { return a = a | b; }

    Compiler() = default;
    explicit Compiler(Flags flags) : m_flags(flags) {}

    void set_flags(Flags flags) { m_flags = flags; }
    Flags flags() const { return m_flags; }

    /// Compile AST into Function object, which contains objects in scope + code
    /// (module is a special kind of function, with predefined parameters)
    /// \returns true if actually compiled (depends on Flags)
    bool compile(Function& func, ast::Module& ast);

    // Compile block, return type of its return value
    void compile_block(Function& func, const ast::Block& block);

private:
    Flags m_flags = Flags::Default;
};


} // namespace xci::script

#endif // include guard
