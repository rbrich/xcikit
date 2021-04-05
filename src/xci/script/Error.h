// Error.h created on 2019-05-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_ERROR_H
#define XCI_SCRIPT_ERROR_H

#include "Value.h"
#include "SourceInfo.h"
#include "dump.h"
#include <xci/core/error.h>

#include <fmt/core.h>
#include <fmt/ostream.h>


namespace xci::script {

using fmt::format;


class ScriptError : public core::Error {
public:
    explicit ScriptError(std::string msg) : Error(std::move(msg)) {}
    explicit ScriptError(std::string msg, const SourceInfo& si) :
        Error(std::move(msg)),
        m_file(format("{}:{}:{}",
            !si.source ? "<no-source-file>" : si.source, si.line_number, si.column)),
        m_detail(!si ? "" : format("{}\n{:>{}}",
            std::string{si.line_begin, si.line_end},
            '^', si.column))
    {}

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
    explicit NotImplemented(const std::string& name)
        : ScriptError(format("not implemented: {}", name)) {}
};


struct BadInstruction : public ScriptError {
    explicit BadInstruction(uint8_t code)
            : ScriptError(format("bad instruction: {}", code)) {}
};


struct ParseError : public ScriptError {
    explicit ParseError(const std::string& msg)
            : ScriptError(format("parse error: {}", msg)) {}
};


struct StackUnderflow : public ScriptError {
    explicit StackUnderflow() : ScriptError(format("stack underflow")) {}
};


struct StackOverflow : public ScriptError {
    explicit StackOverflow() : ScriptError(format("stack overflow")) {}
};


struct UndefinedName : public ScriptError {
    explicit UndefinedName(const std::string& name, const SourceInfo& si)
        : ScriptError(format("undefined name: {}", name), si) {}
};


struct UndefinedTypeName : public ScriptError {
    explicit UndefinedTypeName(const std::string& name)
            : ScriptError(format("undefined type name: {}", name)) {}
};


struct MultipleDeclarationError : public ScriptError {
    explicit MultipleDeclarationError(const std::string& name)
            : ScriptError(format("multiple declaration of name: {}", name)) {}
};


struct UnsupportedOperandsError : public ScriptError {
    explicit UnsupportedOperandsError(const std::string& op)
            : ScriptError(format("unsupported operands to '{}'", op)) {}
};


struct UnknownTypeName : public ScriptError {
    explicit UnknownTypeName(const std::string& name)
        : ScriptError(format("unknown type name: {}", name)) {}
};


struct UnexpectedArgumentCount : public ScriptError {
    explicit UnexpectedArgumentCount(size_t exp, size_t got, const SourceInfo& si)
            : ScriptError(format("function expects {} args, called with {} args",
                    exp, got), si) {}
};


struct UnexpectedArgument : public ScriptError {
    explicit UnexpectedArgument(size_t idx, const SourceInfo& si)
            : ScriptError(format("unexpected argument #{}", idx), si) {}
};


struct UnexpectedArgumentType : public ScriptError {
    explicit UnexpectedArgumentType(size_t idx, const TypeInfo& exp, const TypeInfo& got, const SourceInfo& si)
            : ScriptError(format("function expects {} for arg #{}, called with {}",
                                 exp, idx, got), si) {}
};


struct UnexpectedReturnType : public ScriptError {
    explicit UnexpectedReturnType(const TypeInfo& exp, const TypeInfo& got)
            : ScriptError(format("function returns {}, body evaluates to {}",
                                 exp, got)) {}
};


struct MissingExplicitType : public ScriptError {
    explicit MissingExplicitType()
        : ScriptError("type cannot be inferred and wasn't specified") {}
};


struct FunctionNotFound : public ScriptError {
    explicit FunctionNotFound(const std::string& name, const std::string& args,
                              const std::string& candidates)
        : ScriptError(format("function not found: {} {}\n   Candidates:\n{}", name, args, candidates)) {}
};


struct FunctionConflict : public ScriptError {
    explicit FunctionConflict(const std::string& name, const std::string& args,
                              const std::string& candidates)
            : ScriptError(format("function cannot be uniquely resolved: {} {}\n   Candidates:\n{}", name, args, candidates)) {}
};


struct FunctionNotFoundInClass : public ScriptError {
    explicit FunctionNotFoundInClass(const std::string& fn, const std::string& cls)
            : ScriptError(format("instance function '{}' not found in class '{}'",
                                 fn, cls)) {}
};


struct TooManyLocalsError : public ScriptError {
    explicit TooManyLocalsError()
            : ScriptError(format("too many local values in function")) {}
};


struct ConditionNotBool : public ScriptError {
    explicit ConditionNotBool() : ScriptError("condition doesn't evaluate to Bool") {}
};


struct DefinitionTypeMismatch : public ScriptError {
    explicit DefinitionTypeMismatch(const TypeInfo& exp, const TypeInfo& got)
            : ScriptError(format("definition type mismatch: specified {}, inferred {}",
                                 exp, got)) {}
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


struct IntrinsicsFunctionError : public ScriptError {
    explicit IntrinsicsFunctionError(const std::string& message, const SourceInfo& si)
        : ScriptError("intrinsics function: " + message, si) {}
};


} // namespace xci::script

#endif // include guard
