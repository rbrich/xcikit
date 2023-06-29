// resolve_spec.h created on 2022-08-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_RESOLVE_SPEC_H
#define XCI_SCRIPT_AST_RESOLVE_SPEC_H

#include "AST.h"

namespace xci::script {


/// Resolve generic functions and methods to concrete instances

void resolve_spec(Scope& scope, ast::Expression& body);


} // namespace xci::script

#endif // include guard
