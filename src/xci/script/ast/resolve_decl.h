// resolve_decl.h created on 2022-07-17 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_RESOLVE_DECL_H
#define XCI_SCRIPT_AST_RESOLVE_DECL_H

#include "AST.h"

namespace xci::script {


/// Resolve declared types

void resolve_decl(FunctionScope& scope, const ast::Block& block);


} // namespace xci::script

#endif // include guard
