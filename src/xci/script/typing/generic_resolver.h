// generic_resolver.h created on 2022-12-31 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_GENERIC_RESOLVER_H
#define XCI_SCRIPT_GENERIC_RESOLVER_H

#include "TypeChecker.h"
#include <xci/script/TypeInfo.h>
#include <xci/script/Function.h>
#include <xci/script/Source.h>
#include <functional>

namespace xci::script {

using UnexpectedTypeCallback = std::function<void(const TypeInfo& exp, const TypeInfo& got)>;

void set_type_arg(SymbolPointer var, const TypeInfo& deduced, TypeArgs& type_args,
                  const UnexpectedTypeCallback& exc_cb);
void get_type_arg(SymbolPointer var, TypeInfo& sig, const TypeArgs& type_args);
void copy_type_arg(SymbolPointer var, const TypeArgs& src, TypeArgs& dst);
inline TypeInfo get_type_arg(SymbolPointer var, const TypeArgs& type_args) {
    TypeInfo res;
    get_type_arg(var, res, type_args);
    return res;
}

void resolve_generic_type(TypeInfo& sig, const TypeArgs& type_args);
void resolve_generic_type(TypeInfo& sig, const Scope& scope);

void resolve_type_vars(Signature& signature, const TypeArgs& type_args);
void resolve_type_vars(Signature& signature, const Scope& scope);

/// Check return type matches and set it to concrete type if it's generic
void resolve_return_type(Signature& sig, const TypeInfo& deduced,
                         Scope& scope, const SourceLocation& loc);

void specialize_arg(const TypeInfo& sig, const TypeInfo& deduced,
                    TypeArgs& type_args,
                    const UnexpectedTypeCallback& exc_cb);

/// Check `scope` for TypeVar symbols and store their resolved type to scope
void store_resolved_param_type_vars(Scope& scope, const TypeArgs& type_args);

}  // namespace xci::script

#endif  // include guard
