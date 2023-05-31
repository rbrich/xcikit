// generic_resolver.cpp created on 2022-12-31 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "generic_resolver.h"

#include <range/v3/view/zip.hpp>

namespace xci::script {

using ranges::views::zip;


void set_type_arg(SymbolPointer var, const TypeInfo& deduced, TypeArgs& type_args,
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


void get_type_arg(SymbolPointer var, TypeInfo& sig, const TypeArgs& type_args)
{
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


void resolve_generic_type(TypeInfo& sig, const TypeArgs& type_args)
{
    switch (sig.type()) {
        case Type::Unknown: {
            auto var = sig.generic_var();
            if (var) {
                get_type_arg(var, sig, type_args);
            }
            break;
        }
        case Type::List:
            resolve_generic_type(sig.elem_type(), type_args);
            break;
        case Type::Tuple:
            for (auto& sub : sig.subtypes())
                resolve_generic_type(sub, type_args);
            break;
        case Type::Function:
            sig = TypeInfo(std::make_shared<Signature>(sig.signature()));  // copy
            resolve_generic_type(sig.signature().param_type, type_args);
            resolve_generic_type(sig.signature().return_type, type_args);
            break;
        default:
            // Int32 etc. (never generic)
            break;
    }
}


void resolve_generic_type(TypeInfo& sig, const Scope& scope)
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
            resolve_generic_type(sig.elem_type(), scope);
            break;
        case Type::Tuple:
            for (auto& sub : sig.subtypes())
                resolve_generic_type(sub, scope);
            break;
        case Type::Function:
            sig = TypeInfo(std::make_shared<Signature>(sig.signature()));  // copy
            resolve_generic_type(sig.signature().param_type, scope);
            resolve_generic_type(sig.signature().return_type, scope);
            break;
        default:
            // Int32 etc. (never generic)
            break;
    }
}


void resolve_type_vars(Signature& signature, const TypeArgs& type_args)
{
    resolve_generic_type(signature.param_type, type_args);
    resolve_generic_type(signature.return_type, type_args);
}


void resolve_type_vars(Signature& signature, const Scope& scope)
{
    resolve_generic_type(signature.param_type, scope);
    resolve_generic_type(signature.return_type, scope);
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
            if (deduced.type() != Type::Function) {
                exc_cb(sig, deduced);
                break;
            }
            specialize_arg(sig.signature().param_type, deduced.signature().param_type, type_args, exc_cb);
            specialize_arg(sig.signature().return_type, deduced.signature().return_type, type_args, exc_cb);
            break;
        default:
            // Int32 etc. (never generic)
            break;
    }
}


void store_resolved_param_type_vars(Scope& scope, const TypeArgs& type_args)
{
    auto& symtab = scope.function().symtab();
    for (const auto& s : symtab) {
        const SymbolPointer var = symtab.find(s);
        if (s.type() == Symbol::TypeVar) {
            TypeInfo ti;
            get_type_arg(var, ti, type_args);
            if (ti)
                scope.type_args().set(var, ti);
        }
    }
}


}  // namespace xci::script
