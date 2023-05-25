// fold_paren.h created on 2023-05-25 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_FOLD_PAREN_H
#define XCI_SCRIPT_AST_FOLD_PAREN_H

#include "AST.h"

namespace xci::script {


/// AST with Parenthesized:
///     Parenthesized(Expression)
///        Tuple(Expression)
///           Integer(Expression) 1
///           Parenthesized(Expression)
///              Tuple(Expression)
///                 Integer(Expression) 2
///                 String(Expression) "three"
///
/// Folded into AST without:
///     Tuple(Expression)
///        Integer(Expression) 1
///        Tuple(Expression)
///           Integer(Expression) 2
///           String(Expression) "three"
///
/// Mandatory AST pass (unfolded parentheses won't compile)
void fold_paren(const ast::Block& block);


} // namespace xci::script

#endif // include guard
