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
    enum class Flags: uint32_t {
        // All compiler passes
        // UNSAFE: Don't use these flags directly - the wrong set of flags
        //         can crash the compiler (asserts in Debug mode).
        //         Use PP/CP/OP flags below, which contain required dependencies.
        FoldTuple           = 0x0001u,
        FoldDotCall         = 0x0002u,
        FoldParen           = 0x0004u,
        ResolveSymbols      = 0x0010u,
        ResolveDecl         = 0x0020u,
        ResolveTypes        = 0x0040u,
        ResolveSpec         = 0x0080u,
        ResolveNonlocals    = 0x0100u,
        CompileFunctions    = 0x1000u,
        AssembleFunctions   = 0x2000u,
        FoldConstExpr       = 0x0001u << 16,
        InlineFunctions     = 0x0002u << 16,
        OptimizeCopyDrop    = 0x0004u << 16,
        OptimizeTailCall    = 0x0008u << 16,

        // Bit masks
        MandatoryMask       = 0xffffu,
        OptimizationMask    = 0xffffu << 16,

        // Predefined optimization levels
        OptLevel1       = OptimizeTailCall | OptimizeCopyDrop,
        OptLevel2       = OptLevel1 | FoldConstExpr | InlineFunctions,

        // ---------------------------------------------------------------------
        // The following flags are safe to use individually

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

        // Mandatory compilation passes
        CPCompile       = CompileFunctions | PPNonlocals,
        CPAssemble      = AssembleFunctions | CPCompile,

        // Optimization passes
        OPCopyDrop      = OptimizeCopyDrop | CPCompile,
        OPTailCall      = OptimizeTailCall | CPCompile,

        // All mandatory passes, no optimization
        Mandatory       = CPAssemble,

        // All mandatory passes + optimizations
        O1              = Mandatory | OptLevel1,
        O2              = Mandatory | OptLevel2,

        // Mandatory passes + default optimizations
        Default         = O1
    };

    // Allow basic arithmetic on OpCode
    friend inline constexpr Flags operator~(Flags a) { return Flags(~uint32_t(a)); }
    friend inline constexpr Flags operator|(Flags a, Flags b) { return Flags(uint32_t(a) | uint32_t(b)); }
    friend inline constexpr Flags operator&(Flags a, Flags b) { return Flags(uint32_t(a) & uint32_t(b)); }
    friend inline constexpr Flags operator|=(Flags& a, Flags b) { return a = a | b; }

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

private:
    /// Compile all functions in a module except `main`
    /// that are marked with compile flag but not yet compiled
    void compile_all_functions(Scope& main);

    Flags m_flags = Flags::Default;
};


} // namespace xci::script

#endif // include guard
