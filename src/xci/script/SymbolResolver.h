// SymbolResolver.h created on 2019-06-14, part of XCI toolkit
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

#ifndef XCI_SCRIPT_SYMBOLRESOLVER_H
#define XCI_SCRIPT_SYMBOLRESOLVER_H

#include "AST.h"

namespace xci::script {


/// Walk the AST and search for symbolic references
/// - check for undefined names (throw UndefinedName)
/// - register new names in function's scope
/// - register non-local references
/// - blocks (function bodies) are walked in breadth-first order
///   (this allows references to all parent definitions, not just those
///   preceding the block syntactically)

class SymbolResolver: public ast::BlockProcessor {
public:
    void process_block(Function& func, const ast::Block& block) final;
};


/// Simplify non-local symbol references
/// - multi-level references are flattened to single-level references
///   (by adding the non-locals also to the parent and referencing those)
/// - non-locals referencing functions without closure
///   (those that don't capture anything by themselves)
///   are replaced with locals (the function is referenced directly)

class NonlocalResolver: public ast::BlockProcessor {
public:
    void process_block(Function& func, const ast::Block& block) final;
};


} // namespace xci::script

#endif // include guard
