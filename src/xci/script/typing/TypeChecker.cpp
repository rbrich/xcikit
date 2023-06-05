// TypeChecker.cpp created on 2022-07-24 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "TypeChecker.h"
#include <xci/script/Error.h>

namespace xci::script {


MatchScore match_params(const std::vector<TypeInfo>& candidate, const std::vector<TypeInfo>& expected)
{
    if (candidate.size() != expected.size())
        return MatchScore(-1);
    MatchScore score;
    for (size_t i = 0; i != expected.size(); ++i) {
        auto m = match_type(candidate[i], expected[i]);
        if (!m || m.is_coerce())
            return MatchScore(-1);
        score += m;
    }
    return score;
}


MatchScore match_type(const TypeInfo& candidate, const TypeInfo& expected)
{
    if (candidate.is_struct() && expected.is_struct())
        return match_struct(candidate, expected);
    if (candidate.is_tuple() && expected.is_tuple())
        return match_tuple(candidate, expected);
    if (candidate.is_literal() && candidate.is_tuple() && expected.is_struct())
        return MatchScore::coerce() + match_tuple_to_struct(candidate, expected);
    if (candidate == expected) {
        if (expected.has_unknown() || candidate.has_unknown())
            return MatchScore::generic();
        else
            return MatchScore::exact();
    }
    if (candidate.is_literal() && expected.is_named())
        return MatchScore::coerce() + match_type(candidate, expected.underlying());
    return MatchScore::mismatch();
}


MatchScore match_tuple(const TypeInfo& candidate, const TypeInfo& expected)
{
    assert(candidate.is_tuple());
    assert(expected.is_tuple());
    const auto& expected_types = expected.subtypes();
    const auto& candidate_types = candidate.subtypes();
    if (candidate_types.size() != expected_types.size())
        return MatchScore::mismatch();  // number of fields doesn't match
    if (candidate == expected)
        return MatchScore::exact();
    MatchScore res;
    if (candidate.is_named() || expected.is_named())
        res.add_coerce();
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
    const auto& expected_items = expected.struct_items();
    if (candidate == expected)
        return MatchScore::exact();
    MatchScore res;
    if (candidate.is_named() || expected.is_named()) {
        // The named type doesn't match.
        // The underlying type may match - each field adds another match to total score.
        res.add_coerce();
    }
    for (const auto& inf : candidate.struct_items()) {
        auto act_it = std::find_if(expected_items.begin(), expected_items.end(),
                [&inf](const TypeInfo::StructItem& act) {
                  return act.first == inf.first;
                });
        if (act_it == expected_items.end())
            return MatchScore::mismatch();  // not found
        // check item type
        auto m = match_type(inf.second, act_it->second);
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
    const auto& expected_items = expected.struct_items();
    const auto& candidate_types = candidate.subtypes();
    if (candidate_types.size() != expected_items.size() && !candidate_types.empty())  // allow initializing a struct with ()
        return MatchScore::mismatch();  // number of fields doesn't match
    if (candidate == expected)
        return MatchScore::exact();
    MatchScore res;
    if (candidate.is_named() || expected.is_named())
        res.add_coerce();
    auto expected_iter = expected_items.begin();
    for (const auto& inf_type : candidate_types) {
        auto m = match_type(inf_type, expected_iter->second);
        if (!m)
            return MatchScore::mismatch();  // item type doesn't match
        res += m;
        ++expected_iter;
    }
    return res;
}


auto TypeChecker::resolve(const TypeInfo& inferred, const SourceLocation& loc) const -> TypeInfo
{
    // struct - resolve to either specified or cast type
    const TypeInfo& ti = eval_type();
    if (ti.is_tuple()) {
        if (inferred.is_tuple()) {
            if (!match_tuple(inferred, ti))
                throw DefinitionTypeMismatch(ti, inferred, loc);
            return ti;
        }
    }
    if (ti.is_struct()) {
        if (inferred.is_struct()) {
            if (!match_struct(inferred, ti))
                throw DefinitionTypeMismatch(ti, inferred, loc);
            return ti;
        }
        if (inferred.is_tuple()) {
            if (!match_tuple_to_struct(inferred, ti))
                throw DefinitionTypeMismatch(ti, inferred, loc);
            return ti;
        }
        if (ti.struct_items().size() == 1) {
            // allow initializing a single-field struct with the value of first field (as there is no single-item tuple)
            if (!match_type(inferred, ti.struct_items().front().second))
                throw DefinitionTypeMismatch(ti, inferred, loc);
            return ti;
        }
    }
    if (ti.is_list()) {
        if (inferred.is_list()) {
            if (!match_type(inferred.elem_type(), ti.elem_type()))
                throw DefinitionTypeMismatch(ti, inferred, loc);
            if (ti.elem_type().has_unknown() && !inferred.elem_type().has_unknown())
                return inferred;
            return ti;
        }
    }
    // otherwise, resolve to specified type, ignore cast type (a cast function will be called)
    if (!m_spec)
        return inferred;
    if (!match_type(inferred, m_spec))
        throw DefinitionTypeMismatch(m_spec, inferred, loc);
    return m_spec;
}


void TypeChecker::check_struct_item(const std::string& key, const TypeInfo& inferred, const SourceLocation& loc) const
{
    assert(eval_type().is_struct());
    const auto& spec_items = eval_type().struct_items();
    auto spec_it = std::find_if(spec_items.begin(), spec_items.end(),
                                [&key](const TypeInfo::StructItem& spec) {
                                    return spec.first == key;
                                });
    if (spec_it == spec_items.end())
        throw StructUnknownKey(eval_type(), key, loc);
    if (!match_type(inferred, spec_it->second))
        throw StructKeyTypeMismatch(eval_type(), spec_it->second, inferred, loc);
}


}  // namespace xci::script
