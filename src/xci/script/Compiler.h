// Compiler.h created on 2019-05-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
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
        FoldParen           = 0x0004,
        ResolveSymbols      = 0x0010,
        ResolveDecl         = 0x0020,
        ResolveTypes        = 0x0040,
        ResolveSpec         = 0x0080,
        ResolveNonlocals    = 0x0100,
        CompileFunctions    = 0x1000,
        FoldConstExpr       = 0x0001 << 16,

        // Bit masks
        MandatoryMask       = 0xffff,
        OptimizationMask    = 0xffff << 16,

        // Mandatory AST passes
        // SAFE: these flags bring in also their dependencies
        PPTuple         = FoldTuple,
        PPDotCall       = FoldDotCall,
        PPParen         = FoldParen,
        PPSymbols       = ResolveSymbols | PPDotCall | PPTuple | PPParen,
        PPDecl          = ResolveDecl | PPSymbols,
        PPTypes         = ResolveTypes | PPDecl,
        PPSpec          = ResolveSpec | PPTypes,
        PPNonlocals     = ResolveNonlocals | PPSpec,

        // All mandatory passes, no optimization
        Default         = CompileFunctions | PPNonlocals,

        // Optimization passes
        OP1             = FoldConstExpr,

        // All mandatory passes + optimizations
        O1              = Default | OP1,
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
    /// Compiles only partially unless flags contain complete Default set.
    void compile(Scope& scope, ast::Module& ast);

    /// Compile single function that has fully prepared AST
    /// (all other phases were run on it)
    void compile_function(Scope& scope, ast::Expression& body);

    /// Compile all functions in a module except `main`
    /// that are marked with compile flag but not yet compiled
    void compile_all_functions(Scope& main);

private:
    Flags m_flags = Flags::Default;
};


} // namespace xci::script

#endif // include guard
