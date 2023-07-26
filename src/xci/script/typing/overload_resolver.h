// overload_resolver.h created on 2022-12-31 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_OVERLOAD_RESOLVER_H
#define XCI_SCRIPT_OVERLOAD_RESOLVER_H

#include "TypeChecker.h"
#include <xci/script/TypeInfo.h>
#include <xci/script/Source.h>
#include <xci/script/Function.h>

namespace xci::script {


struct CallArg {
    TypeInfo type_info;
    SourceLocation source_loc;
};


/// Given arguments and expected return type for a called function
struct CallSignature {
    CallArg arg;
    TypeInfo return_type;

    void set_arg(CallArg&& a) { arg = std::move(a); }
    void set_return_type(TypeInfo ti) { return_type = std::move(ti); return_type.set_literal(false); }
    void clear() { arg = {}; return_type = {}; return_type.set_literal(false); }
    bool empty() const noexcept { return !arg.type_info; }

    void load_from(const Signature& sig, const SourceLocation& source_loc) {
        arg = {sig.param_type, source_loc};
        return_type = sig.return_type;
    }

    Signature signature() const noexcept {
        Signature sig;
        sig.param_type = arg.type_info;
        sig.return_type = return_type;
        return sig;
    }
};


struct Candidate {
    Module* module;
    Index scope_index;
    SymbolPointer symptr;
    TypeInfo type;      // Method: instance type
    TypeInfo gen_type;  // Method: class fn type
    TypeArgs type_args;
    MatchScore match;
};


/// Find best match from candidates
std::pair<const Candidate*, bool> find_best_candidate(const std::vector<Candidate>& candidates);

/// Resolve type variables in `signature` according to `call_sig`
TypeArgs specialize_signature(const SignaturePtr& signature, const std::vector<CallSignature>& call_sig_stack, TypeArgs call_type_args = {});

/// Resolve type variables in `call_sig_stack` that are concrete in `signature`
TypeArgs resolve_generic_args_to_signature(const Signature& signature, const std::vector<CallSignature>& call_sig_stack);

/// Match call args with signature (which contains type vars T, U...)
/// Throw if unmatched, return resolved TypeArgs for T, U... if matched
TypeArgs resolve_instance_types(const Signature& signature, const std::vector<CallSignature>& call_sig_stack, const TypeInfo& cast_type);

/// Match signature to call args.
/// \returns total MatchScore of all parameters and return value, or mismatch
/// Partial match is possible when the signature has less parameters than call args.
MatchScore match_signature(const Signature& signature, const std::vector<CallSignature>& call_sig_stack, const TypeInfo& cast_type);

}  // namespace xci::script

#endif  // include guard
