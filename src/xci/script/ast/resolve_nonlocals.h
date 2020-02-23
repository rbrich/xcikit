// resolve_nonlocals.h created on 2020-01-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_RESOLVE_NONLOCALS_H
#define XCI_SCRIPT_AST_RESOLVE_NONLOCALS_H

#include "AST.h"

namespace xci::script {


/// Simplify non-local symbol references
/// - multi-level references are flattened to single-level references
///   (by adding the non-locals also to the parent and referencing those)
/// - non-locals referencing functions without closure
///   (those that don't capture anything by themselves)
///   are replaced with direct references

void resolve_nonlocals(Function& func, const ast::Block& block);


} // namespace xci::script

#endif // include guard
