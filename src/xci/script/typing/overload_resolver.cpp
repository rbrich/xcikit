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
    for (const CallSignature& call_sig : call_sig_stack | reverse) {
        if (!sig)
            sig = signature;
        else if (sig->return_type.is_callable()) {
            // continue with specializing args of a returned function
            sig = sig->return_type.ul_signature_ptr();
        } else {
            throw unexpected_argument(TypeInfo{sig}, call_sig.arg.source_loc);
        }
        // skip blocks / functions without params
        while (sig->param_type.is_void() && sig->return_type.is_callable()) {
            sig = sig->return_type.ul_signature_ptr();
        }
        const auto& c_sig = call_sig.signature();
        const auto& source_loc = call_sig.arg.source_loc;
        {
            if (!sig->has_nonvoid_param() && c_sig.has_nonvoid_param()) {
                throw unexpected_argument(TypeInfo{sig}, source_loc);
            }
            const auto& sig_type = sig->param_type;
            const auto& call_type = c_sig.param_type;
            if (sig_type.has_generic() && !call_type.is_unknown()) {
                specialize_arg(sig_type, call_type,
                               call_type_args,
                               [&sig_type, &call_type, &source_loc]
                               (const TypeInfo& exp, const TypeInfo& got) {
                                   throw unexpected_argument_type(exp, got, sig_type, call_type, source_loc);
                               });
            }
        }
        if (sig->return_type.has_generic()) {
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
            throw unexpected_argument(TypeInfo{std::make_shared<Signature>(*sig)}, call_sig.arg.source_loc);
        }
        // skip blocks / functions without params
        while (sig->param_type.is_void() && sig->return_type.type() == Type::Function) {
            sig = &sig->return_type.signature();
        }
        const auto& c_sig = call_sig.signature();
        const auto& source_loc = call_sig.arg.source_loc;
        {
            // check there are more params to consume
            if (!sig->has_nonvoid_param() && c_sig.has_nonvoid_param()) {
                throw unexpected_argument(TypeInfo{std::make_shared<Signature>(*sig)}, source_loc);
            }
            // next param
            const auto& sig_type = sig->param_type;
            const auto& call_type = c_sig.param_type;
            if (!sig_type.has_generic()) {
                // resolve arg if it's a type var and the signature has a known type in its place
                if (call_type.has_generic()) {
                    specialize_arg(call_type, sig_type,  // NOLINT, args swapped intentionally
                                   param_type_args,
                                   [](const TypeInfo& exp, const TypeInfo& got) {});
                }
            }
        }
    }
    return param_type_args;
}


TypeArgs resolve_instance_types(const Signature& signature, const std::vector<CallSignature>& call_sig_stack, const TypeInfo& cast_type)
{
    const Signature* sig = nullptr;
    TypeArgs res;
    for (const CallSignature& call_sig : call_sig_stack | reverse) {
        if (sig == nullptr)
            sig = &signature;
        else if (sig->return_type.type() == Type::Function) {
            // collapse returned function, start consuming its params
            sig = &sig->return_type.signature();
        } else {
            throw unexpected_argument(TypeInfo{std::make_shared<Signature>(*sig)}, call_sig.arg.source_loc);
        }
        // skip blocks / functions without params
        while (sig->param_type.is_void() && sig->return_type.type() == Type::Function) {
            sig = &sig->return_type.signature();
        }
        // resolve args
        const auto& c_sig = call_sig.signature();
        const auto& source_loc = call_sig.arg.source_loc;
        {
            // check there are more params to consume
            if (!sig->has_nonvoid_param() && c_sig.has_nonvoid_param()) {
                // unexpected argument
                throw unexpected_argument(TypeInfo{std::make_shared<Signature>(*sig)}, source_loc);
            }
            // resolve T (only from original signature)
            const auto& sig_type = sig->param_type;
            auto call_type = c_sig.param_type;

            if (call_type.is_struct() && sig_type.is_tuple()) {
                // downgrade struct to tuple in call_type
                call_type.set_type(Type::Tuple);
            }

            const auto m = match_type(call_type, sig_type);
            if (!m)
                throw unexpected_argument_type(sig_type, call_type, source_loc);

            auto arg_type = call_type.effective_type();
            specialize_arg(sig_type, arg_type, res,
                    [&sig_type, &arg_type, &source_loc]
                    (const TypeInfo& exp, const TypeInfo& got) {
                        throw unexpected_argument_type(exp, got, sig_type, arg_type, source_loc);
                    });
        }
    }
    // use m_call_ret only as a hint - if return type var is still unknown
    if (signature.return_type.is_unknown()) {
        auto var = signature.return_type.generic_var();
        assert(var);
        if (!call_sig_stack.empty() && !call_sig_stack[0].return_type.is_unknown()) {
            set_type_arg(var, call_sig_stack[0].return_type, res,
                         [](const TypeInfo& exp, const TypeInfo& got) {});
        }
        if (!cast_type.is_unknown()) {
            set_type_arg(var, cast_type.effective_type(), res,
                         [](const TypeInfo& exp, const TypeInfo& got) {});
        }
    }
    return res;
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
        while (sig->param_type.is_void() && sig->return_type.type() == Type::Function) {
            sig = &sig->return_type.signature();
        }
        const auto& c_sig = call_sig.signature();
        {
            // check type of next param
            const auto& sig_type = sig->param_type;
            const auto& call_type = c_sig.param_type;
            auto m = match_type(call_type, sig_type);
            if (!m)
                return MatchScore::mismatch();
            res += m;
        }
        // check return type
        if (call_sig.return_type) {
            auto m = match_type(call_sig.return_type, sig->return_type);
            if (!m || m.is_coerce())
                return MatchScore::mismatch();
            res += m;
        }
    }
    if ((call_sig_stack.empty() || call_sig_stack.back().empty()) && signature.param_type.is_void())
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
