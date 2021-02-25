// fold_tuple.h created on 2021-02-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_FOLD_TUPLE_H
#define XCI_SCRIPT_AST_FOLD_TUPLE_H

#include "AST.h"

namespace xci::script {


/// Tuple is parsed as comma operator, leading to following AST,
/// given input tuple (1, 2, "three"):
///     Bracketed(Expression)
///        OpCall(Expression)
///           Operator , [L2]
///           OpCall(Expression)
///              Operator , [L2]
///              Integer(Expression) 1
///              Integer(Expression) 2
///           String(Expression) "three"
///
/// The goal is to translate to AST of this form:
///     Tuple(Expression)
///        Integer(Expression) 1
///        Integer(Expression) 2
///        String(Expression) "three"
///
/// The same is done for lists:
///     List(Expression)
///        OpCall(Expression)
///           Operator , [L2]
///              Integer(Expression) 1
///              Integer(Expression) 2
///
/// Is folded to:
///     List(Expression)
///        Integer(Expression) 1
///        Integer(Expression) 2
///
/// Mandatory AST pass (unfolded tuples and lists won't compile)
void fold_tuple(const ast::Block& block);


} // namespace xci::script

#endif // include guard
