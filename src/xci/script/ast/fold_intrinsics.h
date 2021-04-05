// fold_intrinsics.h created on 2021-04-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_FOLD_INTRINSICS_H
#define XCI_SCRIPT_AST_FOLD_INTRINSICS_H

#include "AST.h"

namespace xci::script {


/// Intrinsics look like normal functions, but they are translated
/// to opcodes and their arguments. The arguments need to be removed from AST,
/// because they won't be pushed onto stack, but instead added directly into code.
///
/// Given input: __drop 8 4
///
/// Original AST:
///
///    Call(Expression)
///       Reference(Expression)
///          Identifier __drop [Instruction 78 @builtin]
///       Literal(Expression) 8
///       Literal(Expression) 4
///
/// Folded AST:
///
///    Call(Expression)
///       Reference(Expression)
///          Identifier __drop [Instruction 78 @builtin]
///
/// The instruction args are now hidden in the Reference node, not visible in AST dump.
///
/// Mandatory AST pass (unfolded intrinsics won't compile)
void fold_intrinsics(const ast::Block& block);


} // namespace xci::script

#endif  // include guard
