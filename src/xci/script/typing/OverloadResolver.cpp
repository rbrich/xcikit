// OverloadResolver.cpp created on 2022-12-31 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "OverloadResolver.h"
#include "GenericResolver.h"

namespace xci::script {


std::pair<const Candidate*, bool> find_best_candidate(const std::vector<Candidate>& candidates)
{
    bool conflict = false;
    MatchScore score = MatchScore::mismatch();
    const Candidate* found = nullptr;
    for (const auto& item : candidates) {
        if (!item.match)
            continue;
        if (item.match > score) {
            // found better match
            score = item.match;
            found = &item;
            conflict = false;
            continue;
        }
        if (item.match == score) {
            // found same match -> conflict
            conflict = true;
        }
    }
    return {found, conflict};
}


TypeArgs specialize_signature(const SignaturePtr& signature, const CallSignature& call_sig, TypeArgs call_type_args)
{
    auto sig = signature;
    size_t i = 0;
    size_t arg_n = 1;
    for (const auto& arg : call_sig.args) {
        while (i >= sig->params.size()) {
            if (sig->return_type.type() == Type::Function) {
                // continue with specializing args of a returned function
                sig = sig->return_type.signature_ptr();
                i = 0;
            } else {
                throw UnexpectedArgument(arg_n, TypeInfo{signature}, arg.source_loc);
            }
        }
        const auto& sig_type = sig->params[i++];
        //            if (arg.type_info.is_unknown())
        //                continue;
        if (sig_type.is_generic()) {
            specialize_arg(sig_type, arg.type_info,
                           call_type_args,
                           [arg_n, &arg](const TypeInfo& exp, const TypeInfo& got) {
                               throw UnexpectedArgumentType(arg_n, exp, got, arg.source_loc);
                           });
        }
        ++arg_n;
    }
    if (sig->return_type.is_generic()) {
        specialize_arg(sig->return_type, call_sig.return_type,
                       call_type_args,
                       [](const TypeInfo& exp, const TypeInfo& got) {});
    }
    return call_type_args;
}


TypeArgs resolve_generic_args_to_signature(const Signature& signature, const CallSignature& call_sig)
{
    auto* sig = &signature;
    size_t i = 0;
    TypeArgs param_type_args;
    for (const auto& arg : call_sig.args) {
        // check there are more params to consume
        while (i >= sig->params.size()) {
            if (sig->return_type.type() == Type::Function) {
                // continue resolving params of returned function
                sig = &sig->return_type.signature();
                i = 0;
            } else {
                throw UnexpectedArgument(i+1, TypeInfo{std::make_shared<Signature>(signature)}, arg.source_loc);
            }
        }
        // next param
        const auto& sig_type = sig->params[i++];
        if (!sig_type.is_generic()) {
            // resolve arg if it's a type var and the signature has a known type in its place
            if (arg.type_info.is_generic()) {
                specialize_arg(arg.type_info, sig_type,
                               param_type_args,
                               [i, &arg](const TypeInfo& exp, const TypeInfo& got) {
                                   throw UnexpectedArgumentType(i, exp, got, arg.source_loc);
                               });
            }
        }
    }
    return param_type_args;
}


}  // namespace xci::script
