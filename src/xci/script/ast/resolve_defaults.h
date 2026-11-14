// resolve_defaults.h created on 2026-11-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2026 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_RESOLVE_DEFAULTS_H
#define XCI_SCRIPT_AST_RESOLVE_DEFAULTS_H

#include "AST.h"

namespace xci::script {


/// Resolve default values in struct types.
/// The defaults must be constant expressions, otherwise throw StructDefaultNotConstant.
/// Store the default values as module statics.
/// Should run after fold_const_expr so that constant expressions have been folded to literals.

void resolve_defaults(Scope& scope, const ast::Block& block);


} // namespace xci::script

#endif // include guard
