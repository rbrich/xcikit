// TypeChecker.cpp created on 2022-07-24 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "TypeChecker.h"
#include <xci/script/Error.h>

namespace xci::script {


MatchScore match_params(const std::vector<TypeInfo>& candidate, const std::vector<TypeInfo>& actual)
{
    if (candidate.size() != actual.size())
        return MatchScore(-1);
    MatchScore score;
    for (size_t i = 0; i != actual.size(); ++i) {
        auto m = match_type(candidate[i], actual[i]);
        if (!m || m.is_coerce())
            return MatchScore(-1);
        score += m;
    }
    return score;
}


MatchScore match_type(const TypeInfo& candidate, const TypeInfo& actual)
{
    if (candidate.is_struct() && actual.is_struct())
        return match_struct(candidate, actual);
    if (candidate.is_tuple() && actual.is_tuple())
        return match_tuple(candidate, actual);
    if (candidate.is_tuple() && actual.is_struct())
        return MatchScore::coerce() + match_tuple_to_struct(candidate, actual);
    if (candidate == actual) {
        if (actual.is_unknown_or_generic() || candidate.is_unknown_or_generic())
            return MatchScore::generic();
        else
            return MatchScore::exact();
    }
    if (candidate.is_named() || actual.is_named())
        return MatchScore::coerce() + match_type(candidate.underlying(), actual.underlying());
    return MatchScore::mismatch();
}


MatchScore match_tuple(const TypeInfo& candidate, const TypeInfo& actual)
{
    assert(candidate.is_tuple());
    assert(actual.is_tuple());
    const auto& actual_types = actual.subtypes();
    const auto& candidate_types = candidate.subtypes();
    if (candidate_types.size() != actual_types.size())
        return MatchScore::mismatch();  // number of fields doesn't match
    if (candidate == actual)
        return MatchScore::exact();
    MatchScore res;
    if (candidate.is_named() || actual.is_named())
        res.add_coerce();
    auto actual_iter = actual_types.begin();
    for (const auto& inf_type : candidate_types) {
        auto m = match_type(inf_type, *actual_iter);
        if (!m)
            return MatchScore::mismatch();  // item type doesn't match
        res += m;
        ++actual_iter;
    }
    return res;
}


MatchScore match_struct(const TypeInfo& candidate, const TypeInfo& actual)
{
    assert(candidate.is_struct());
    assert(actual.is_struct());
    const auto& actual_items = actual.struct_items();
    if (candidate == actual)
        return MatchScore::exact();
    MatchScore res;
    if (candidate.is_named() || actual.is_named()) {
        // The named type doesn't match.
        // The underlying type may match - each field adds another match to total score.
        res.add_coerce();
    }
    for (const auto& inf : candidate.struct_items()) {
        auto act_it = std::find_if(actual_items.begin(), actual_items.end(),
                [&inf](const TypeInfo::StructItem& act) {
                  return act.first == inf.first;
                });
        if (act_it == actual_items.end())
            return MatchScore::mismatch();  // not found
        // check item type
        auto m = match_type(inf.second, act_it->second);
        if (!m)
            return MatchScore::mismatch();  // item type doesn't match
        res += m;
    }
    return res;
}


MatchScore match_tuple_to_struct(const TypeInfo& candidate, const TypeInfo& actual)
{
    assert(candidate.is_tuple());
    assert(actual.is_struct());
    const auto& actual_items = actual.struct_items();
    const auto& candidate_types = candidate.subtypes();
    if (candidate_types.size() != actual_items.size() && !candidate_types.empty())  // allow initializing a struct with ()
        return MatchScore::mismatch();  // number of fields doesn't match
    if (candidate == actual)
        return MatchScore::exact();
    MatchScore res;
    if (candidate.is_named() || actual.is_named())
        res.add_coerce();
    auto actual_iter = actual_items.begin();
    for (const auto& inf_type : candidate_types) {
        auto m = match_type(inf_type, actual_iter->second);
        if (!m)
            return MatchScore::mismatch();  // item type doesn't match
        res += m;
        ++actual_iter;
    }
    return res;
}


auto TypeChecker::resolve(const TypeInfo& inferred, const SourceLocation& loc) const -> TypeInfo
{
    // struct - resolve to either specified or cast type
    const TypeInfo& ti = eval_type();
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
            if (ti.elem_type().is_unknown_or_generic() && !inferred.elem_type().is_unknown_or_generic())
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
