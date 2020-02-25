// fold_const_expr.h created on 2019-06-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_FOLD_CONST_EXPR_H
#define XCI_SCRIPT_AST_FOLD_CONST_EXPR_H

#include "AST.h"

namespace xci::script {


/// Optimize AST by folding constant expressions

void fold_const_expr(Function& func, const ast::Block& block);


} // namespace xci::script

#endif // include guard
