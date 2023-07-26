// Error.h created on 2019-05-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_ERROR_H
#define XCI_SCRIPT_ERROR_H

#include "Value.h"
#include "Source.h"
#include <xci/core/error.h>
#include <xci/core/TermCtl.h>

#include <fmt/core.h>
#include <fmt/ostream.h>

#include <string_view>


namespace xci::script {

using core::TermCtl;
using std::string_view;


class ScriptError : public core::Error {
public:
    explicit ScriptError(std::string msg) : Error(std::move(msg)) {}
    explicit ScriptError(std::string msg, const SourceLocation& loc) :
        Error(std::move(msg)),
        m_file(fmt::format("{}:{}:{}",
            loc.source_name(), loc.line, loc.column))
    {
        if (!loc)
            return;
        auto line = loc.source_line();
        auto column = TermCtl::stripped_width(std::string_view{line}.substr(0, loc.column));
        m_detail = fmt::format("{}\n{:>{}}", line, '^', column);
    }

    const std::string& file() const noexcept { return m_file; }
    const std::string& detail() const noexcept { return m_detail; }

private:
    std::string m_file;
    std::string m_detail;
};


struct StackTraceFrame {
    std::string function_name;
};

using StackTrace = std::vector<StackTraceFrame>;


class RuntimeError : public ScriptError {
public:
    explicit RuntimeError(std::string msg) : ScriptError(std::move(msg)) {}

    void set_stack_trace(StackTrace&& trace) { m_trace = std::move(trace); }
    const StackTrace& trace() const noexcept { return m_trace; }

private:
    StackTrace m_trace;
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


struct NotImplemented : public RuntimeError {
    explicit NotImplemented(string_view name)
        : RuntimeError(fmt::format("not implemented: {}", name)) {}
};


struct ValueOutOfRange : public RuntimeError {
    explicit ValueOutOfRange(std::string msg) : RuntimeError(std::move(msg)) {}
};


struct IndexOutOfBounds : public RuntimeError {
    explicit IndexOutOfBounds(int idx, size_t len)
            : RuntimeError(fmt::format("list index out of bounds: {} not in [0..{}]",
                                       idx, len-1)) {}
};


struct BadInstruction : public ScriptError {
    explicit BadInstruction(uint8_t code)
            : ScriptError(fmt::format("bad instruction: {}", code)) {}
};


struct ParseError : public ScriptError {
    explicit ParseError(string_view msg, const SourceLocation& loc = {})
            : ScriptError(fmt::format("parse error: {}", msg), loc) {}
};


struct StackUnderflow : public ScriptError {
    explicit StackUnderflow() : ScriptError(fmt::format("stack underflow")) {}
};


struct StackOverflow : public ScriptError {
    explicit StackOverflow() : ScriptError(fmt::format("stack overflow")) {}
};


struct UndefinedName : public ScriptError {
    explicit UndefinedName(string_view name, const SourceLocation& loc)
        : ScriptError(fmt::format("undefined name: {}", name), loc) {}
};


struct UndefinedTypeName : public ScriptError {
    explicit UndefinedTypeName(string_view name, const SourceLocation& loc)
            : ScriptError(fmt::format("undefined type name: {}", name), loc) {}
};


struct RedefinedName : public ScriptError {
    explicit RedefinedName(string_view name, const SourceLocation& loc)
            : ScriptError(fmt::format("redefined name: {}", name), loc) {}
};


struct UnsupportedOperandsError : public ScriptError {
    explicit UnsupportedOperandsError(string_view op)
            : ScriptError(fmt::format("unsupported operands to '{}'", op)) {}
};


struct UnknownTypeName : public ScriptError {
    explicit UnknownTypeName(string_view name)
        : ScriptError(fmt::format("unknown type name: {}", name)) {}
};


struct UnexpectedArgument : public ScriptError {
    explicit UnexpectedArgument(const TypeInfo& ftype, const SourceLocation& loc);
};


struct UnexpectedArgumentType : public ScriptError {
    explicit UnexpectedArgumentType(const TypeInfo& exp, const TypeInfo& got,
                                    const SourceLocation& loc);

    explicit UnexpectedArgumentType(const TypeInfo& exp, const TypeInfo& got,
                                    const TypeInfo& exp_arg, const TypeInfo& got_arg,
                                    const SourceLocation& loc);
};


struct UnexpectedReturnType : public ScriptError {
    explicit UnexpectedReturnType(const TypeInfo& exp, const TypeInfo& got);
};


struct MissingExplicitType : public ScriptError {
    explicit MissingExplicitType(const SourceLocation& loc)
        : ScriptError("type cannot be inferred and wasn't specified", loc) {}
    explicit MissingExplicitType(string_view name, const SourceLocation& loc)
            : ScriptError(fmt::format("type cannot be inferred and wasn't specified: {}", name), loc) {}
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
    explicit UnexpectedGenericFunction(string_view fn_desc)
            : ScriptError(fmt::format("unexpected generic function: {}", fn_desc)) {}
    explicit UnexpectedGenericFunction(string_view fn_desc, const SourceLocation& loc)
            : ScriptError(fmt::format("unexpected generic function: {}", fn_desc), loc) {}
};


struct FunctionNotFound : public ScriptError {
    explicit FunctionNotFound(string_view fn_desc,
                              string_view candidates, const SourceLocation& loc)
        : ScriptError(fmt::format("function not found: {}\n"
                             "   Candidates:\n{}", fn_desc, candidates), loc) {}
};


struct FunctionConflict : public ScriptError {
    explicit FunctionConflict(string_view fn_desc,
                              string_view candidates, const SourceLocation& loc)
            : ScriptError(fmt::format("function cannot be uniquely resolved: {}\n"
                                 "   Candidates:\n{}", fn_desc, candidates), loc) {}
};


struct FunctionNotFoundInClass : public ScriptError {
    explicit FunctionNotFoundInClass(string_view fn, string_view cls)
            : ScriptError(fmt::format("instance function '{}' not found in class '{}'",
                                 fn, cls)) {}
};


struct TooManyLocals : public ScriptError {
    explicit TooManyLocals()
            : ScriptError(fmt::format("too many local values in function")) {}
};


struct ConditionNotBool : public ScriptError {
    explicit ConditionNotBool() : ScriptError("condition doesn't evaluate to Bool") {}
};


struct DeclarationTypeMismatch : public ScriptError {
    explicit DeclarationTypeMismatch(const TypeInfo& decl, const TypeInfo& now, const SourceLocation& loc);
};


struct DefinitionTypeMismatch : public ScriptError {
    explicit DefinitionTypeMismatch(const TypeInfo& exp, const TypeInfo& got, const SourceLocation& loc);
};


struct DefinitionParamTypeMismatch : public ScriptError {
    explicit DefinitionParamTypeMismatch(size_t idx, const TypeInfo& exp, const TypeInfo& got, const SourceLocation& loc);
};


struct BranchTypeMismatch : public ScriptError {
    explicit BranchTypeMismatch(const TypeInfo& exp, const TypeInfo& got);
};


struct ListElemTypeMismatch : public ScriptError {
    explicit ListElemTypeMismatch(const TypeInfo& exp, const TypeInfo& got, const SourceLocation& loc);
};


struct StructTypeMismatch : public ScriptError {
    explicit StructTypeMismatch(const TypeInfo& got, const SourceLocation& loc);
};


struct StructUnknownKey : public ScriptError {
    explicit StructUnknownKey(const TypeInfo& struct_type, const std::string& key, const SourceLocation& loc);
};


struct StructDuplicateKey : public ScriptError {
    explicit StructDuplicateKey(const std::string& key, const SourceLocation& loc)
            : ScriptError(fmt::format("duplicate struct key \"{}\"", key), loc) {}
};


struct StructKeyTypeMismatch : public ScriptError {
    explicit StructKeyTypeMismatch(const TypeInfo& struct_type, const TypeInfo& spec, const TypeInfo& got, const SourceLocation& loc);
};


struct IntrinsicsFunctionError : public ScriptError {
    explicit IntrinsicsFunctionError(string_view message, const SourceLocation& loc)
        : ScriptError(fmt::format("intrinsics function: {}", message), loc) {}
};


struct UnresolvedSymbol : public ScriptError {
    explicit UnresolvedSymbol(string_view name)
            : ScriptError(fmt::format("unresolved symbol: {}", name)) {}
};


struct ImportError : public ScriptError {
    explicit ImportError(string_view name)
            : ScriptError(fmt::format("module not found: {}", name)) {}
};


} // namespace xci::script

#endif // include guard
