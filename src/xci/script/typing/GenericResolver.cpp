// GenericResolver.cpp created on 2022-12-31 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "GenericResolver.h"

#include <range/v3/view/zip.hpp>

namespace xci::script {

using ranges::views::zip;


void resolve_generic_type(const TypeArgs& type_args, TypeInfo& sig)
{
    switch (sig.type()) {
        case Type::Unknown: {
            auto var = sig.generic_var();
            if (var) {
                for (;;) {
                    auto ti = type_args.get(var);
                    if (ti.is_unknown() && ti.generic_var()) {
                        sig = ti;
                        var = ti.generic_var();
                        continue;
                    }
                    if (ti) {
                        sig = ti;
                    }
                    break;
                }
            }
            break;
        }
        case Type::List:
            resolve_generic_type(type_args, sig.elem_type());
            break;
        case Type::Tuple:
            for (auto& sub : sig.subtypes())
                resolve_generic_type(type_args, sub);
            break;
        case Type::Function:
            sig = TypeInfo(std::make_shared<Signature>(sig.signature()));  // copy
            for (auto& prm : sig.signature().params)
                resolve_generic_type(type_args, prm);
            resolve_generic_type(type_args, sig.signature().return_type);
            break;
        default:
            // Int32 etc. (never generic)
            break;
    }
}


void resolve_generic_type(const Scope& scope, TypeInfo& sig)
{
    switch (sig.type()) {
        case Type::Unknown: {
            auto var = sig.generic_var();
            if (var) {
                const Scope* scope_p = &scope;
                for (;;) {
                    auto ti = scope_p->type_args().get(var);
                    if (ti.is_unknown() && ti.generic_var()) {
                        sig = ti;
                        var = ti.generic_var();
                        scope_p = &scope;
                        continue;
                    }
                    if (ti) {
                        sig = ti;
                        break;
                    }
                    if (!scope_p->parent())
                        break;
                    scope_p = scope_p->parent();
                }
            }
            break;
        }
        case Type::List:
            resolve_generic_type(scope, sig.elem_type());
            break;
        case Type::Tuple:
            for (auto& sub : sig.subtypes())
                resolve_generic_type(scope, sub);
            break;
        case Type::Function:
            sig = TypeInfo(std::make_shared<Signature>(sig.signature()));  // copy
            for (auto& prm : sig.signature().params)
                resolve_generic_type(scope, prm);
            resolve_generic_type(scope, sig.signature().return_type);
            break;
        default:
            // Int32 etc. (never generic)
            break;
    }
}


void resolve_type_vars(Signature& signature, const TypeArgs& type_args)
{
    for (auto& arg_type : signature.params) {
        resolve_generic_type(type_args, arg_type);
    }
    resolve_generic_type(type_args, signature.return_type);
}


static void set_type_arg(SymbolPointer var, const TypeInfo& deduced, TypeArgs& type_args,
                  const std::function<void(const TypeInfo& exp, const TypeInfo& got)>& exc_cb)
{
    auto [it, inserted] = type_args.set(var, deduced);
    if (!inserted) {
        TypeInfo& existing = it->second;
        if (!match_type(existing, deduced))
            exc_cb(existing, deduced);
        if (existing.is_unknown()) {
            if (existing.generic_var())
                set_type_arg(existing.generic_var(), deduced, type_args, exc_cb);
            else
                existing = deduced;
        }
    }
}


void specialize_arg(const TypeInfo& sig, const TypeInfo& deduced,
                    TypeArgs& type_args,
                    const std::function<void(const TypeInfo& exp, const TypeInfo& got)>& exc_cb)
{
    switch (sig.type()) {
        case Type::Unknown: {
            auto var = sig.generic_var();
            if (var) {
                set_type_arg(var, deduced, type_args, exc_cb);
            }
            break;
        }
        case Type::List:
            if (deduced.type() != Type::List) {
                exc_cb(sig, deduced);
                break;
            }
            specialize_arg(sig.elem_type(), deduced.elem_type(), type_args, exc_cb);
            break;
        case Type::Tuple:
            if (deduced.type() != Type::Tuple || sig.subtypes().size() != deduced.subtypes().size()) {
                exc_cb(sig, deduced);
                break;
            }
            for (auto&& [sig_sub, deduced_sub] : zip(sig.subtypes(), deduced.subtypes())) {
                specialize_arg(sig_sub, deduced_sub, type_args, exc_cb);
            }
            break;
        case Type::Function:
            if (deduced.type() != Type::Function || sig.signature().arity() != deduced.signature().arity()) {
                exc_cb(sig, deduced);
                break;
            }
            for (auto&& [sig_arg, deduced_arg] : zip(sig.signature().params, deduced.signature().params)) {
                specialize_arg(sig_arg, deduced_arg, type_args, exc_cb);
            }
            specialize_arg(sig.signature().return_type, deduced.signature().return_type, type_args, exc_cb);
            break;
        default:
            // Int32 etc. (never generic)
            break;
    }
}


}  // namespace xci::script
