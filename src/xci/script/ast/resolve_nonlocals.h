// resolve_nonlocals.h created on 2020-01-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_RESOLVE_NONLOCALS_H
#define XCI_SCRIPT_AST_RESOLVE_NONLOCALS_H

#include "AST.h"

namespace xci::script {


/// Add non-local symbol references (i.e. captured values for building a closure):
/// - for referenced parameters from outer scopes
/// - for referenced functions with closure (functions that have non-locals themselves)
///
/// In case of multi-level references, we have to add intermediate non-locals:
/// - add the same non-local symbol also to the parent scope
/// - reference the non-local in parent scope instead of the target symbol directly

void resolve_nonlocals(Scope& main, ast::Expression& body);


} // namespace xci::script

#endif // include guard
