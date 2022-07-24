// resolve_symbols.h created on 2019-06-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_RESOLVE_SYMBOLS_H
#define XCI_SCRIPT_AST_RESOLVE_SYMBOLS_H

#include "AST.h"

namespace xci::script {


/// Walk the AST and search for symbolic references
/// - check for undefined names (throw UndefinedName)
/// - register new names in function's scope
/// - register non-local references
/// - blocks (function bodies) are walked in breadth-first order
///   (this allows references to all parent definitions, not just those
///   preceding the block syntactically)

void resolve_symbols(FunctionScope& scope, const ast::Block& block);


} // namespace xci::script

#endif // include guard
