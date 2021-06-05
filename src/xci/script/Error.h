// Error.h created on 2019-05-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_ERROR_H
#define XCI_SCRIPT_ERROR_H

#include "Value.h"
#include "Source.h"
#include "dump.h"
#include <xci/core/error.h>
#include <xci/core/TermCtl.h>

#include <fmt/core.h>
#include <fmt/ostream.h>

#include <string_view>


namespace xci::script {

using core::TermCtl;
using fmt::format;
using std::string_view;


class ScriptError : public core::Error {
public:
    explicit ScriptError(std::string msg) : Error(std::move(msg)) {}
    explicit ScriptError(std::string msg, const SourceLocation& loc) :
        Error(std::move(msg)),
        m_file(format("{}:{}:{}",
            loc.source_name(), loc.line, loc.column))
    {
        if (!loc)
            return;
        auto line = loc.source_line();
        auto column = TermCtl::stripped_width(std::string_view{line}.substr(0, loc.column));
        m_detail = format("{}\n{:>{}}", line, '^', column);
    }

    const std::string& file() const noexcept { return m_file; }
    const std::string& detail() const noexcept { return m_detail; }

private:
    std::string m_file;
    std::string m_detail;
};


inline std::ostream& operator<<(std::ostream& os, const ScriptError& e) noexcept
{
    if (!e.file().empty())
        os << e.file() << ": ";
    os << "Error: " << e.what();
    if (!e.detail().empty())
        os << std::endl << e.detail();
    return os;
}


struct NotImplemented : public ScriptError {
    explicit NotImplemented(string_view name)
        : ScriptError(format("not implemented: {}", name)) {}
};


struct BadInstruction : public ScriptError {
    explicit BadInstruction(uint8_t code)
            : ScriptError(format("bad instruction: {}", code)) {}
};


struct ParseError : public ScriptError {
    explicit ParseError(string_view msg, const SourceLocation& loc = {})
            : ScriptError(format("parse error: {}", msg), loc) {}
};


struct StackUnderflow : public ScriptError {
    explicit StackUnderflow() : ScriptError(format("stack underflow")) {}
};


struct StackOverflow : public ScriptError {
    explicit StackOverflow() : ScriptError(format("stack overflow")) {}
};


struct UndefinedName : public ScriptError {
    explicit UndefinedName(string_view name, const SourceLocation& loc)
        : ScriptError(format("undefined name: {}", name), loc) {}
};


struct UndefinedTypeName : public ScriptError {
    explicit UndefinedTypeName(string_view name, const SourceLocation& loc)
            : ScriptError(format("undefined type name: {}", name), loc) {}
};


struct RedefinedName : public ScriptError {
    explicit RedefinedName(string_view name, const SourceLocation& loc)
            : ScriptError(format("redefined name: {}", name), loc) {}
};


struct UnsupportedOperandsError : public ScriptError {
    explicit UnsupportedOperandsError(string_view op)
            : ScriptError(format("unsupported operands to '{}'", op)) {}
};


struct UnknownTypeName : public ScriptError {
    explicit UnknownTypeName(string_view name)
        : ScriptError(format("unknown type name: {}", name)) {}
};


struct UnexpectedArgumentCount : public ScriptError {
    explicit UnexpectedArgumentCount(size_t exp, size_t got, const SourceLocation& loc)
            : ScriptError(format("function expects {} args, called with {} args",
                    exp, got), loc) {}
};


struct UnexpectedArgument : public ScriptError {
    explicit UnexpectedArgument(size_t idx, const SourceLocation& loc)
            : ScriptError(format("unexpected argument #{}", idx), loc) {}
};


struct UnexpectedArgumentType : public ScriptError {
    // num is 1-based
    explicit UnexpectedArgumentType(size_t num, const TypeInfo& exp, const TypeInfo& got, const SourceLocation& loc)
            : ScriptError(format("function expects {} for arg #{}, called with {}",
                                 exp, num, got), loc) {}
};


struct UnexpectedReturnType : public ScriptError {
    explicit UnexpectedReturnType(const TypeInfo& exp, const TypeInfo& got)
            : ScriptError(format("function returns {}, body evaluates to {}",
                                 exp, got)) {}
};


struct MissingExplicitType : public ScriptError {
    explicit MissingExplicitType(const SourceLocation& loc)
        : ScriptError("type cannot be inferred and wasn't specified", loc) {}
};

struct MissingTypeArg : public ScriptError {
    explicit MissingTypeArg(const SourceLocation& loc)
        : ScriptError("generic function requires type argument", loc) {}
};

struct UnexpectedTypeArg : public ScriptError {
    explicit UnexpectedTypeArg(const SourceLocation& loc)
            : ScriptError("unexpected type argument", loc) {}
};

struct UnexpectedGenericFunction : public ScriptError {
    explicit UnexpectedGenericFunction(const SourceLocation& loc)
            : ScriptError("generic function must be named or immediately called", loc) {}
};


struct FunctionNotFound : public ScriptError {
    explicit FunctionNotFound(string_view name, string_view args,
                              string_view candidates, const SourceLocation& loc)
        : ScriptError(format("function not found: {} {}\n"
                             "   Candidates:\n{}", name, args, candidates), loc) {}
};


struct FunctionConflict : public ScriptError {
    explicit FunctionConflict(string_view name, string_view args,
                              string_view candidates, const SourceLocation& loc)
            : ScriptError(format("function cannot be uniquely resolved: {} {}\n"
                                 "   Candidates:\n{}", name, args, candidates), loc) {}
};


struct FunctionNotFoundInClass : public ScriptError {
    explicit FunctionNotFoundInClass(string_view fn, string_view cls)
            : ScriptError(format("instance function '{}' not found in class '{}'",
                                 fn, cls)) {}
};


struct TooManyLocals : public ScriptError {
    explicit TooManyLocals()
            : ScriptError(format("too many local values in function")) {}
};


struct ConditionNotBool : public ScriptError {
    explicit ConditionNotBool() : ScriptError("condition doesn't evaluate to Bool") {}
};


struct DeclarationTypeMismatch : public ScriptError {
    explicit DeclarationTypeMismatch(const TypeInfo& decl, const TypeInfo& now, const SourceLocation& loc)
            : ScriptError(format("declared type mismatch: previous {}, this {}",
            decl, now), loc) {}
};


struct DefinitionTypeMismatch : public ScriptError {
    explicit DefinitionTypeMismatch(const TypeInfo& exp, const TypeInfo& got, const SourceLocation& loc)
            : ScriptError(format("definition type mismatch: specified {}, inferred {}",
                                 exp, got), loc) {}
};


struct DefinitionParamTypeMismatch : public ScriptError {
    explicit DefinitionParamTypeMismatch(size_t idx, const TypeInfo& exp, const TypeInfo& got)
            : ScriptError(format("definition type mismatch: specified {} for param #{}, inferred {}",
                                 exp, idx, got)) {}
};


struct BranchTypeMismatch : public ScriptError {
    explicit BranchTypeMismatch(const TypeInfo& exp, const TypeInfo& got)
            : ScriptError(format("branch type mismatch: then branch {} else branch {}",
                                 exp, got)) {}
};


struct ListElemTypeMismatch : public ScriptError {
    explicit ListElemTypeMismatch(const TypeInfo& exp, const TypeInfo& got)
            : ScriptError(format("list element type mismatch: got {} in list of {}",
                                 got, exp)) {}
};


struct IndexOutOfBounds : public ScriptError {
    explicit IndexOutOfBounds(int idx, size_t len)
            : ScriptError(format("list index out of bounds: {} not in [0..{}]",
                                 idx, len-1)) {}
};


struct StructTypeMismatch : public ScriptError {
    explicit StructTypeMismatch(const TypeInfo& got, const SourceLocation& loc)
            : ScriptError(format("cannot cast a struct initializer to {}",
            got), loc) {}
};


struct StructUnknownKey : public ScriptError {
    explicit StructUnknownKey(const TypeInfo& struct_type, const std::string& key, const SourceLocation& loc)
            : ScriptError(format("struct key \"{}\" doesn't match struct type {}",
            key, struct_type), loc) {}
};


struct StructDuplicateKey : public ScriptError {
    explicit StructDuplicateKey(const std::string& key, const SourceLocation& loc)
            : ScriptError(format("duplicate struct key \"{}\"", key), loc) {}
};


struct StructKeyTypeMismatch : public ScriptError {
    explicit StructKeyTypeMismatch(const TypeInfo& struct_type, const TypeInfo& spec, const TypeInfo& got, const SourceLocation& loc)
            : ScriptError(format("struct item type mismatch: specified {} in {}, inferred {}",
            spec, struct_type, got), loc) {}
};


struct IntrinsicsFunctionError : public ScriptError {
    explicit IntrinsicsFunctionError(string_view message, const SourceLocation& loc)
        : ScriptError(format("intrinsics function: {}", message), loc) {}
};


} // namespace xci::script

#endif // include guard
