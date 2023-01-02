// GenericResolver.h created on 2022-12-31 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_GENERIC_RESOLVER_H
#define XCI_SCRIPT_GENERIC_RESOLVER_H

#include "TypeChecker.h"
#include <xci/script/TypeInfo.h>
#include <xci/script/Function.h>
#include <xci/script/Source.h>
#include <functional>

namespace xci::script {


void get_type_arg(SymbolPointer var, TypeInfo& sig, const TypeArgs& type_args);

void resolve_generic_type(TypeInfo& sig, const TypeArgs& type_args);
void resolve_generic_type(TypeInfo& sig, const Scope& scope);

void resolve_type_vars(Signature& signature, const TypeArgs& type_args);
void resolve_type_vars(Signature& signature, const Scope& scope);

void specialize_arg(const TypeInfo& sig, const TypeInfo& deduced,
                    TypeArgs& type_args,
                    const std::function<void(const TypeInfo& exp, const TypeInfo& got)>& exc_cb);


}  // namespace xci::script

#endif  // include guard
