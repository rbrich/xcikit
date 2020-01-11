// NonlocalResolver.h created on 2020-01-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_NONLOCAL_RESOLVER_H
#define XCI_SCRIPT_NONLOCAL_RESOLVER_H

#include "AST.h"

namespace xci::script {


/// Simplify non-local symbol references
/// - multi-level references are flattened to single-level references
///   (by adding the non-locals also to the parent and referencing those)
/// - non-locals referencing functions without closure
///   (those that don't capture anything by themselves)
///   are replaced with locals (the function is referenced directly)

class NonlocalResolver: public ast::BlockProcessor {
public:
    void process_block(Function& func, const ast::Block& block) final;
};


} // namespace xci::script

#endif // include guard
