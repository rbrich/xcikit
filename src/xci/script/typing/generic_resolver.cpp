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
        if (existing.is_unspecified()) {
            existing = deduced;
        } else if (existing.has_generic()) {
            specialize_arg(existing, deduced, type_args, exc_cb);
            resolve_generic_type(existing, type_args);
        }
    }
}


void get_type_arg(SymbolPointer var, TypeInfo& sig, const TypeArgs& type_args)
{
    for (;;) {
        auto ti = type_args.get(var);
        if (ti.is_generic()) {
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
        case Type::Struct:
            for (auto& st : sig.struct_items())
                resolve_generic_type(st.second, type_args);
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
                    if (ti.is_generic()) {
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
        case Type::Struct:
            for (auto& st : sig.struct_items())
                resolve_generic_type(st.second, scope);
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


void resolve_return_type(Signature& sig, const TypeInfo& deduced,
                         Scope& scope, const SourceLocation& loc)
{
    if (sig.return_type.has_unknown()) {
        if (deduced.is_unknown() && !deduced.has_generic()) {
            if (!sig.has_any_generic())
                throw MissingExplicitType(loc);
            return;  // nothing to resolve
        }
        if (deduced.is_callable() && &sig == &deduced.signature())
            throw MissingExplicitType(loc);  // the return type is recursive!
        specialize_arg(sig.return_type, deduced, scope.type_args(),
                [&loc](const TypeInfo& exp, const TypeInfo& got) {
                    throw UnexpectedReturnType(exp, got, loc);
                });
        resolve_type_vars(sig, scope.type_args());  // fill in concrete types using new type var info
        sig.set_return_type(deduced);  // Unknown/var=0 not handled by resolve_type_vars
        return;
    }
    if (sig.return_type.effective_type() != deduced.effective_type())
        throw UnexpectedReturnType(sig.return_type, deduced, loc);
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
            if (deduced.type() == Type::Tuple) {
                if (sig.subtypes().size() != deduced.subtypes().size()) {
                    exc_cb(sig, deduced);
                    break;
                }
                for (auto&& [sig_sub, deduced_sub] : zip(sig.subtypes(), deduced.subtypes())) {
                    specialize_arg(sig_sub, deduced_sub, type_args, exc_cb);
                }
            } else if (deduced.type() == Type::Struct) {
                if (sig.subtypes().size() != deduced.struct_items().size()) {
                    exc_cb(sig, deduced);
                    break;
                }
                for (auto&& [sig_sub, deduced_st] : zip(sig.subtypes(), deduced.struct_items())) {
                    specialize_arg(sig_sub, deduced_st.second, type_args, exc_cb);
                }
            } else {
                exc_cb(sig, deduced);
            }
            break;
        case Type::Struct:
            if (deduced.type() == Type::Struct) {
                if (sig.struct_items().size() != deduced.struct_items().size()) {
                    exc_cb(sig, deduced);
                    break;
                }
                for (auto&& [sig_st, deduced_st] : zip(sig.struct_items(), deduced.struct_items())) {
                    specialize_arg(sig_st.second, deduced_st.second, type_args, exc_cb);
                }
            } else if (deduced.type() == Type::Tuple) {
                if (sig.struct_items().size() != deduced.subtypes().size()) {
                    exc_cb(sig, deduced);
                    break;
                }
                for (auto&& [sig_st, deduced_sub] : zip(sig.struct_items(), deduced.subtypes())) {
                    specialize_arg(sig_st.second, deduced_sub, type_args, exc_cb);
                }
            } else {
                exc_cb(sig, deduced);
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
