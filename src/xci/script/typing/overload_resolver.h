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
    bool literal_value;
};


/// Given arguments and expected return type for a called function
struct CallSignature {
    std::vector<CallArg> args;
    TypeInfo return_type;

    void add_arg(CallArg&& arg) { args.push_back(std::move(arg)); }
    void clear() { args.clear(); return_type = {}; }
    bool empty() const noexcept { return args.empty(); }
    size_t n_args() const noexcept { return args.size(); }

    void load_from(const Signature& sig, const SourceLocation& source_loc) {
        args.clear();
        for (const auto& p : sig.params)
            args.push_back({p, source_loc, false});
        return_type = sig.return_type;
    }

    Signature signature() const noexcept {
        Signature sig;
        for (const auto& p : args)
            sig.params.push_back(p.type_info);
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
TypeArgs specialize_signature(const SignaturePtr& signature, const CallSignature& call_sig, TypeArgs call_type_args = {});

/// Resolve type variables in `call_args` that are concrete in `signature`
TypeArgs resolve_generic_args_to_signature(const Signature& signature, const CallSignature& call_sig);

}  // namespace xci::script

#endif  // include guard
