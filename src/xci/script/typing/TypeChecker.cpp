// TypeChecker.cpp created on 2022-07-24 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "TypeChecker.h"
#include <xci/script/Error.h>

#include <range/v3/view/zip.hpp>

namespace xci::script {

using ranges::views::zip;


MatchScore match_inst_types(std::span<const TypeInfo> candidate, std::span<const TypeInfo> expected)
{
    if (candidate.size() != expected.size())
        return MatchScore::mismatch();
    MatchScore score;
    for (size_t i = 0; i != expected.size(); ++i) {
        auto m = match_type(candidate[i], expected[i]);
        if (!m || m.is_coerce())
            return MatchScore::mismatch();
        score += m;
    }
    return score;
}


static MatchScore match_named(const TypeInfo& candidate, const TypeInfo& expected)
{
    if (candidate.name() != expected.name())
        return MatchScore::mismatch();
    return match_type(candidate.named_type().type_info, expected.named_type().type_info);
}


static MatchScore match_function(const TypeInfo& candidate, const TypeInfo& expected)
{
    const auto& cnd_sig = candidate.signature();
    const auto& exp_sig = expected.signature();
    if (cnd_sig.nonlocals.size() != exp_sig.nonlocals.size())
        return MatchScore::mismatch();
    MatchScore res;
    for (auto&& [cnd, exp] : zip(cnd_sig.nonlocals, exp_sig.nonlocals)) {
        const auto m = match_type(cnd, exp);
        if (!m)
            return MatchScore::mismatch();
        res += m;
    }
    {
        const auto m = match_type(cnd_sig.param_type, exp_sig.param_type);
        if (!m)
            return MatchScore::mismatch();
        res += m;
    }
    {
        const auto m = match_type(cnd_sig.return_type, exp_sig.return_type);
        if (!m)
            return MatchScore::mismatch();
        res += m;
    }
    return res;
}


MatchScore match_type(const TypeInfo& candidate, const TypeInfo& expected)
{
    if (candidate.is_literal() && candidate.is_tuple() && expected.underlying().is_struct())
        return MatchScore::coerce() + match_tuple_to_struct(candidate, expected.underlying());
    if (candidate.is_unknown() || expected.is_unknown())
        return MatchScore::generic();
    if (candidate.type() == expected.type()) {
        switch (candidate.type()) {
            case Type::List: return match_type(candidate.elem_type(), expected.elem_type());
            case Type::Tuple: return match_tuple(candidate, expected);
            case Type::Struct: return match_struct(candidate, expected);
            case Type::Function: return match_function(candidate, expected);
            case Type::Named: return match_named(candidate, expected);
            default:
                return MatchScore::exact();
        }
    }
    if (candidate.is_literal() && expected.is_named())
        return MatchScore::coerce() + match_type(candidate, expected.underlying());
    return MatchScore::mismatch();
}


MatchScore match_tuple(const TypeInfo& candidate, const TypeInfo& expected)
{
    assert(candidate.is_struct_or_tuple());
    assert(expected.is_struct_or_tuple());
    if (candidate.is_void() && expected.is_void())
        return MatchScore::exact();
    const auto& expected_types = expected.subtypes();
    const auto& candidate_types = candidate.subtypes();
    if (candidate_types.size() != expected_types.size())
        return MatchScore::mismatch();  // number of fields doesn't match
    MatchScore res;
    auto expected_iter = expected_types.begin();
    for (const auto& inf_type : candidate_types) {
        auto m = match_type(inf_type, *expected_iter);
        if (!m)
            return MatchScore::mismatch();  // item type doesn't match
        res += m;
        ++expected_iter;
    }
    return res;
}


MatchScore match_struct(const TypeInfo& candidate, const TypeInfo& expected)
{
    assert(candidate.is_struct());
    assert(expected.is_struct());
    const auto& expected_types = expected.subtypes();
    const auto& candidate_types = candidate.subtypes();
    if (candidate_types.size() > expected_types.size())
        return MatchScore::mismatch();  // number of fields doesn't match
    assert(!expected_types.empty());
    MatchScore res;
    for (const auto& inf_type : candidate_types) {
        assert(inf_type.key());
        auto exp_it = std::find_if(expected_types.begin(), expected_types.end(),
                                   [&inf_type](const TypeInfo& exp) {
                                       return exp.key() == inf_type.key();
                                   });
        if (exp_it == expected_types.end())
            return MatchScore::mismatch();  // not found
        auto m = match_type(inf_type, *exp_it);
        if (!m)
            return MatchScore::mismatch();  // item type doesn't match
        res += m;
    }
    return res;
}


MatchScore match_tuple_to_struct(const TypeInfo& candidate, const TypeInfo& expected)
{
    assert(candidate.is_tuple());
    assert(expected.is_struct());
    if (candidate.is_void())  // allow initializing a struct with ()
        return MatchScore::coerce();
    return match_tuple(candidate, expected);
}


auto TypeChecker::resolve(const TypeInfo& inferred, const SourceLocation& loc) -> TypeInfo
{
    // struct - resolve to either specified or cast type
    const TypeInfo& ti = eval_type();
    const TypeInfo& underlying = ti.underlying();
    if (underlying.is_tuple()) {
        if (inferred.is_tuple()) {
            if (!match_tuple(inferred, underlying))
                throw definition_type_mismatch(ti, inferred, loc);
            return std::move(eval_type());
        }
    }
    if (underlying.is_struct()) {
        if (inferred.is_struct()) {
            if (!match_struct(inferred, underlying))
                throw definition_type_mismatch(ti, inferred, loc);
            return std::move(eval_type());
        }
        if (inferred.is_tuple()) {
            if (!match_tuple_to_struct(inferred, underlying))
                throw definition_type_mismatch(ti, inferred, loc);
            TypeInfo res = std::move(eval_type());
            for (auto&& [st_item, inf_subtype] : zip(res.underlying().subtypes(), inferred.subtypes())) {
                if (st_item.is_unspecified()) {
                    auto key = st_item.key();
                    st_item = inf_subtype;
                    st_item.set_key(key);
                }
            }
            return res;
        }
        if (underlying.subtypes().size() == 1) {
            // allow initializing a single-field struct with the value of first field (as there is no single-item tuple)
            if (!match_type(inferred, underlying.subtypes().front()))
                throw definition_type_mismatch(ti, inferred, loc);
            return std::move(eval_type());
        }
    }
    if (underlying.is_list()) {
        if (inferred.is_list()) {
            if (!match_type(inferred.elem_type(), underlying.elem_type()))
                throw definition_type_mismatch(ti, inferred, loc);
            if (underlying.elem_type().has_unknown() && !inferred.elem_type().has_unknown())
                return inferred;
            return std::move(eval_type());
        }
    }
    // otherwise, resolve to specified type, ignore cast type (a cast function will be called)
    if (!m_spec)
        return inferred;
    if (!match_type(inferred, m_spec))
        throw definition_type_mismatch(m_spec, inferred, loc);
    return std::move(m_spec);
}


void TypeChecker::check_struct_item(NameId key, const TypeInfo& inferred, const SourceLocation& loc) const
{
    const auto& spec_items = eval_type().underlying().subtypes();
    auto spec_it = std::find_if(spec_items.begin(), spec_items.end(),
                                [key](const TypeInfo& spec) {
                                    return spec.key() == key;
                                });
    if (spec_it == spec_items.end())
        throw struct_unknown_key(eval_type(), key, loc);
    if (!match_type(inferred, *spec_it))
        throw struct_key_type_mismatch(eval_type(), *spec_it, inferred, loc);
}


}  // namespace xci::script
