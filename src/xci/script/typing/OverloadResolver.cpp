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


TypeArgs specialize_signature(const std::shared_ptr<Signature>& signature, const CallSignature& call_sig)
{
    auto sig = signature;
    TypeArgs call_type_args;
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
                           [](const TypeInfo& exp, const TypeInfo& got) {});
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


}  // namespace xci::script
