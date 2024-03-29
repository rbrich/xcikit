// resolve_types.h created on 2019-06-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_RESOLVE_TYPES_H
#define XCI_SCRIPT_AST_RESOLVE_TYPES_H

#include "AST.h"

namespace xci::script {


/// Check and propagate type information

void resolve_types(Scope& scope, const ast::Block& block);


} // namespace xci::script

#endif // include guard
