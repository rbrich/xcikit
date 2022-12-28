// TypeChecker.h created on 2022-07-24 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCIKIT_TYPECHECKER_H
#define XCIKIT_TYPECHECKER_H

#include "TypeInfo.h"
#include "Source.h"
#include <compare>

namespace xci::script {


class MatchScore {
public:
    MatchScore() = default;
    MatchScore(int8_t exact, int8_t coerce, int8_t generic)
            : m_exact(exact), m_coerce(coerce), m_generic(generic) {}
    explicit MatchScore(int8_t exact) : m_exact(exact) {}  // -1 => mismatch

    static MatchScore exact() { return MatchScore(1); }
    static MatchScore coerce() { return MatchScore(0, 1, 0); }
    static MatchScore generic() { return MatchScore(0, 0, 1); }
    static MatchScore mismatch() { return MatchScore(-1); }

    void add_exact() { ++m_exact; }
    void add_coerce() { ++m_coerce; }
    void add_generic() { ++m_generic; }

    bool is_exact() const { return m_exact >= 0 && (m_coerce + m_generic) == 0; }
    bool is_coerce() const { return m_coerce > 0; }
    bool is_generic() const { return m_generic > 0; }

    explicit operator bool() const { return m_exact != -1; }
    auto operator<=>(const MatchScore&) const = default;

    MatchScore operator+(MatchScore rhs) {
        return {int8_t(m_exact + rhs.m_exact),
                int8_t(m_coerce + rhs.m_coerce),
                int8_t(m_generic + rhs.m_generic)};
    }

    void operator+=(MatchScore rhs) {
        m_exact += rhs.m_exact;  // NOLINT
        m_coerce += rhs.m_coerce;  // NOLINT
        m_generic += rhs.m_generic;  // NOLINT
    }

    friend std::ostream& operator<<(std::ostream& os, MatchScore v) {
        if (!v)
            return os << "[ ]";
        os << '[' << int(v.m_exact);
        if (v.m_coerce != 0) os << ',' << int(v.m_coerce) << '~';
        if (v.m_generic != 0) os << ',' << int(v.m_generic) << '?';
        return os << ']';
    }

private:
    int8_t m_exact = 0;  // num parameters matched exactly (Int == Int)
    int8_t m_coerce = 0;  // num parameters that can coerce (Int32 => Int64)
    int8_t m_generic = 0;  // num parameters matched generically (T == T or T == Int or Num T == Int)
};

/// Match function parameters
/// \param candidate    Candidate parameters (inferred types of the arguments)
/// \param actual       Actual parameters (expected by signature)
MatchScore match_params(const std::vector<TypeInfo>& candidate, const std::vector<TypeInfo>& actual);

/// Match a single type
/// \param candidate    Candidate type (inferred type)
/// \param actual       Actual type (expected / specified type)
/// \returns MatchScore: mismatch/generic/exact or combination in case of complex types
MatchScore match_type(const TypeInfo& candidate, const TypeInfo& actual);

/// Match tuple to tuple
/// \param candidate    Candidate tuple type
/// \param actual       Actual resolved tuple type for the value
/// \returns Total match score of all fields, or mismatch
MatchScore match_tuple(const TypeInfo& candidate, const TypeInfo& actual);

/// Match incomplete Struct type from ast::StructInit to resolved Struct type.
/// All keys and types from inferred are checked against resolved.
/// Partial match is possible when inferred has less keys than resolved.
/// \param candidate    Possibly incomplete Struct type as constructed from AST
/// \param actual       Actual resolved type for the value
/// \returns Total match score of all fields, or mismatch
MatchScore match_struct(const TypeInfo& candidate, const TypeInfo& actual);

/// Match tuple to resolved Struct type, i.e. initialize struct with tuple literal
/// \param candidate    Tuple with same number of fields
/// \param actual       Actual resolved struct type for the value
/// \returns Total match score of all fields, or mismatch
MatchScore match_tuple_to_struct(const TypeInfo& candidate, const TypeInfo& actual);


class TypeChecker {
public:
    explicit TypeChecker(TypeInfo&& spec) : m_spec(std::move(spec)) {}
    TypeChecker(TypeInfo&& spec, TypeInfo&& cast) : m_spec(std::move(spec)), m_cast(std::move(cast)) {}

    TypeInfo resolve(const TypeInfo& inferred, const SourceLocation& loc) const;

    void check_struct_item(const std::string& key, const TypeInfo& inferred, const SourceLocation& loc) const;

    const TypeInfo& spec() const { return m_spec; }
    TypeInfo&& spec() { return std::move(m_spec); }

    const TypeInfo& cast() const { return m_cast; }
    TypeInfo&& cast() { return std::move(m_cast); }

    const TypeInfo& eval_type() const { return m_cast ? m_cast : m_spec; }
    TypeInfo&& eval_type() { return m_cast ? std::move(m_cast) : std::move(m_spec); }

private:
    TypeInfo m_spec;  // specified type
    TypeInfo m_cast;  // casted-to type
};


}  // namespace xci::script

#endif  // include guard
