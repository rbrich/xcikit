// overload_resolver.cpp created on 2022-12-31 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "overload_resolver.h"
#include "generic_resolver.h"
#include <range/v3/view/reverse.hpp>

namespace xci::script {

using ranges::cpp20::views::reverse;


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


TypeArgs specialize_signature(const SignaturePtr& signature, const std::vector<CallSignature>& call_sig_stack, TypeArgs call_type_args)
{
    SignaturePtr sig;
    size_t arg_n = 1;
    for (const CallSignature& call_sig : call_sig_stack | reverse) {
        if (!sig)
            sig = signature;
        else if (sig->return_type.type() == Type::Function) {
            // continue with specializing args of a returned function
            sig = sig->return_type.signature_ptr();
        } else {
            throw UnexpectedArgument(arg_n, TypeInfo{signature}, call_sig.args[0].source_loc);
        }
        size_t i = 0;
        // skip blocks / functions without params
        while (sig->params.empty() && sig->return_type.type() == Type::Function) {
            sig = sig->return_type.signature_ptr();
        };
        const std::vector<TypeInfo>& params =
                (call_sig.args.size() > 1) ? sig->params[0].subtypes()
                                           : sig->params;
        for (const auto& arg : call_sig.args) {
            if (i >= params.size()) {
                throw UnexpectedArgument(arg_n, TypeInfo{signature}, arg.source_loc);
            }
            const auto& sig_type = params[i++];
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
    }
    return call_type_args;
}


TypeArgs resolve_generic_args_to_signature(const Signature& signature,
                                           const std::vector<CallSignature>& call_sig_stack)
{
    const Signature* sig = nullptr;
    TypeArgs param_type_args;
    for (const CallSignature& call_sig : call_sig_stack | reverse) {
        if (sig == nullptr)
            sig = &signature;
        else if (sig->return_type.type() == Type::Function) {
            // continue resolving params of returned function
            sig = &sig->return_type.signature();
        } else {
            throw UnexpectedArgument(0, TypeInfo{std::make_shared<Signature>(signature)}, call_sig.args[0].source_loc);
        }
        size_t i = 0;
        // skip blocks / functions without params
        while (sig->params.empty() && sig->return_type.type() == Type::Function) {
            sig = &sig->return_type.signature();
        };
        const std::vector<TypeInfo>& params =
                (call_sig.args.size() > 1) ? sig->params[0].subtypes()
                                           : sig->params;
        for (const auto& arg : call_sig.args) {
            // check there are more params to consume
            if (i >= params.size()) {
                throw UnexpectedArgument(i+1, TypeInfo{std::make_shared<Signature>(signature)}, arg.source_loc);
            }
            // next param
            const auto& sig_type = params[i++];
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
    }
    return param_type_args;
}


MatchScore match_signature(const Signature& signature,
                           const std::vector<CallSignature>& call_sig_stack,
                           const TypeInfo& cast_type)
{
    const Signature* sig = nullptr;
    MatchScore res;
    for (const CallSignature& call_sig : call_sig_stack | reverse) {
        if (sig == nullptr)
            sig = &signature;
        else if (sig->return_type.type() == Type::Function) {
            // collapse returned function, start consuming its params
            sig = &sig->return_type.signature();
        } else {
            // unexpected argument
            return MatchScore::mismatch();
        }
        // skip blocks / functions without params
        while (sig->params.empty() && sig->return_type.type() == Type::Function) {
            sig = &sig->return_type.signature();
        };
        const std::vector<TypeInfo>& params =
                (call_sig.args.size() > 1) ? sig->params[0].subtypes()
                                           : sig->params;
        size_t i = 0;
        for (const auto& arg : call_sig.args) {
            // check there are more params to consume
            if (i >= params.size()) {
                // unexpected argument
                return MatchScore::mismatch();
            }
            // check type of next param
            auto m = match_type(arg.type_info, params[i]);
            if (!m || (!arg.literal_value && m.is_coerce()))
                return MatchScore::mismatch();
            res += m;
            // consume next param
            ++i;
        }
        if (i == params.size()) {
            // increase score for full match - whole signature matches the call args
            res.add_exact();
        }
        // check return type
        if (call_sig.return_type) {
            auto m = match_type(call_sig.return_type, sig->return_type);
            if (!m || m.is_coerce())
                return MatchScore::mismatch();
            res += m;
        }
    }
    if ((call_sig_stack.empty() || call_sig_stack.back().empty()) && signature.params.empty())
        res += MatchScore::exact();  // void param + no call args
    if (cast_type) {
        // increase score if casting target type matches return type,
        // but don't fail if it doesn't match
        if (sig == nullptr)
            sig = &signature;
        auto m = match_type(cast_type, sig->return_type);
        if (m)
            res += m;
    }
    return res;
}


}  // namespace xci::script
