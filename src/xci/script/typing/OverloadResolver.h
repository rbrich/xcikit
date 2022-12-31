// OverloadResolver.h created on 2022-12-31 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_OVERLOAD_RESOLVER_H
#define XCI_SCRIPT_OVERLOAD_RESOLVER_H

#include "TypeChecker.h"
#include <xci/script/TypeInfo.h>
#include <xci/script/Source.h>

namespace xci::script {


struct CallArg {
    TypeInfo type_info;
    SourceLocation source_loc;
    SymbolPointer symptr;  // needed for specialization of a generic function passed as an arg
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
            args.push_back({p, source_loc, {}, false});
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
    TypeInfo type;
    MatchScore match;
};


/// Find best match from candidates
std::pair<const Candidate*, bool> find_best_candidate(const std::vector<Candidate>& candidates);


}  // namespace xci::script

#endif  // include guard
