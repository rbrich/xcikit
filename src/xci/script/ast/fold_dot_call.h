// fold_dot_call.h created on 2020-01-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_FOLD_DOT_CALL_H
#define XCI_SCRIPT_AST_FOLD_DOT_CALL_H

#include "AST.h"

namespace xci::script {


/// Dot call is parsed as ordinary operator, with a normal call
/// on right-hand side. This AST pass moves the arguments from
/// the inner Call into outer OpCall, folding one level in the tree
/// and fixing order of the arguments (first arg is on left-hand side
/// of the dot operator).
///
/// Mandatory AST pass (unfolded dot calls won't compile)

void fold_dot_call(Function& func, const ast::Block& block);


} // namespace xci::script

#endif // include guard
