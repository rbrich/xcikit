// Compiler.h created on 2019-05-30, part of XCI toolkit
// Copyright 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef XCI_SCRIPT_COMPILER_H
#define XCI_SCRIPT_COMPILER_H

#include "AST.h"
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

        // parse & process only, do no compile into bytecode
        PPMask      = 3 << 24,
        PPSymbols   = 1 << 24,    // stop after SymbolResolver pass
        PPNonlocals = 2 << 24,    // stop after NonlocalSymbolResolver pass
        PPTypes     = 3 << 24,    // stop after TypeResolver pass
    };

    Compiler();
    explicit Compiler(uint32_t flags) : Compiler() { configure(flags); }

    void configure(uint32_t flags);

    // Compile AST into Function object, which contains objects in scope + code
    // (module is special kind of function, with predefined parameters)
    void compile(Function& func, AST& ast);

    // Compile block, return type of its return value
    void compile_block(Function& func, const ast::Block& block);

    // Module being compiled
    Module& module() { return *m_modules[0]; }

private:
    std::vector<std::unique_ptr<Module>> m_modules;
    std::vector<std::unique_ptr<ast::BlockProcessor>> m_ast_passes;  // preprocessing & optimization passes
};


} // namespace xci::script

#endif // include guard
