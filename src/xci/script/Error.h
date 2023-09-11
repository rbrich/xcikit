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


enum ErrorCode {
    // RuntimeError
    NotImplemented,
    ValueOutOfRange,
    IndexOutOfBounds,
    ModuleNotFound,
    BadInstruction,
    StackUnderflow,
    StackOverflow,

    // compilation errors
    ParseError,
    UndefinedName,
    UndefinedTypeName,
    RedefinedName,
    UnsupportedOperandsError,
    UnknownTypeName,
    UnexpectedArgument,
    UnexpectedArgumentType,
    UnexpectedReturnType,
    MissingExplicitType,
    MissingTypeArg,
    UnexpectedTypeArg,
    UnexpectedGenericFunction,
    FunctionNotFound,
    FunctionConflict,
    FunctionNotFoundInClass,
    TooManyLocals,
    ConditionNotBool,
    DeclarationTypeMismatch,
    DefinitionTypeMismatch,
    DefinitionParamTypeMismatch,
    BranchTypeMismatch,
    ListTypeMismatch,
    ListElemTypeMismatch,
    StructTypeMismatch,
    StructUnknownKey,
    StructDuplicateKey,
    StructKeyTypeMismatch,
    IntrinsicsFunctionError,
    UnresolvedSymbol,
    ImportError,
};


std::ostream& operator<<(std::ostream& os, ErrorCode v);


class ScriptError : public core::Error {
public:
    explicit ScriptError(ErrorCode code, std::string msg) : Error(std::move(msg)), m_code(code) {}
    explicit ScriptError(ErrorCode code, std::string msg, const SourceLocation& loc) :
        Error(std::move(msg)),
        m_file(fmt::format("{}:{}:{}", loc.source_name(), loc.line, loc.column)),
        m_code(code)
    {
        if (!loc)
            return;
        auto line = loc.source_line();
        auto column = TermCtl::stripped_width(std::string_view{line}.substr(0, loc.column));
        m_detail = fmt::format("{}\n{:>{}}", line, '^', column);
    }

    const std::string& file() const noexcept { return m_file; }
    const std::string& detail() const noexcept { return m_detail; }
    ErrorCode code() const noexcept { return m_code; }

private:
    std::string m_file;
    std::string m_detail;
    ErrorCode m_code;
};


struct StackTraceFrame {
    std::string function_name;
};

using StackTrace = std::vector<StackTraceFrame>;


class RuntimeError : public ScriptError {
public:
    explicit RuntimeError(ErrorCode code, std::string msg) : ScriptError(code, std::move(msg)) {}

    void set_stack_trace(StackTrace&& trace) { m_trace = std::move(trace); }
    const StackTrace& trace() const noexcept { return m_trace; }

private:
    StackTrace m_trace;
};


inline std::ostream& operator<<(std::ostream& os, const ScriptError& e) noexcept
{
    if (!e.file().empty())
        os << e.file() << ": ";
    os << e.code() << ": " << e.what();
    if (!e.detail().empty())
        os << std::endl << e.detail();
    return os;
}


inline RuntimeError not_implemented(string_view name) {
    return RuntimeError(ErrorCode::NotImplemented, fmt::format("not implemented: {}", name));
}

inline RuntimeError value_out_of_range(std::string msg) {
    return RuntimeError(ErrorCode::ValueOutOfRange, std::move(msg));
}

inline RuntimeError index_out_of_bounds(int64_t idx, size_t len) {
    return RuntimeError(ErrorCode::IndexOutOfBounds,
                        fmt::format("list index out of bounds: {} not in [0..{}]", idx, len-1));
}

inline RuntimeError module_not_found(string_view name) {
    return RuntimeError(ErrorCode::ModuleNotFound,
                        fmt::format("imported module not found: {}", name));
}

inline RuntimeError bad_instruction(uint8_t code) {
    return RuntimeError(ErrorCode::BadInstruction, fmt::format("bad instruction: {}", code));
}

inline RuntimeError bad_instruction(std::string msg) {
    return RuntimeError(ErrorCode::BadInstruction, std::move(msg));
}

inline RuntimeError stack_underflow() {
    return RuntimeError(ErrorCode::StackUnderflow, fmt::format("stack underflow"));
}

inline RuntimeError stack_overflow() {
    return RuntimeError(ErrorCode::StackOverflow, fmt::format("stack overflow"));
}


inline ScriptError parse_error(string_view msg, const SourceLocation& loc = {}) {
    return ScriptError(ErrorCode::ParseError, std::string(msg), loc);
}

inline ScriptError undefined_name(string_view name, const SourceLocation& loc) {
    return ScriptError(ErrorCode::UndefinedName, fmt::format("undefined name: {}", name), loc);
}

inline ScriptError undefined_type_name(string_view name, const SourceLocation& loc) {
    return ScriptError(ErrorCode::UndefinedTypeName,
                       fmt::format("undefined type name: {}", name), loc);
}

inline ScriptError redefined_name(string_view name, const SourceLocation& loc) {
    return ScriptError(ErrorCode::RedefinedName, fmt::format("redefined name: {}", name), loc);
}

inline ScriptError unsupported_operands_error(string_view op) {
    return ScriptError(ErrorCode::UnsupportedOperandsError,
                       fmt::format("unsupported operands to '{}'", op));
}

inline ScriptError unknown_type_name(string_view name) {
    return ScriptError(ErrorCode::UnknownTypeName, fmt::format("unknown type name: {}", name));
}

ScriptError unexpected_argument(const TypeInfo& ftype, const SourceLocation& loc);


ScriptError unexpected_argument_type(const TypeInfo& exp, const TypeInfo& got,
                                     const SourceLocation& loc);
ScriptError unexpected_argument_type(const TypeInfo& exp, const TypeInfo& got,
                                     const TypeInfo& exp_arg, const TypeInfo& got_arg,
                                     const SourceLocation& loc);

ScriptError unexpected_return_type(const TypeInfo& exp, const TypeInfo& got, const SourceLocation& loc);



inline ScriptError missing_explicit_type(const SourceLocation& loc) {
    return ScriptError(ErrorCode::MissingExplicitType,
                       "type cannot be inferred and wasn't specified", loc);
}

inline ScriptError missing_explicit_type(string_view name, const SourceLocation& loc) {
    return ScriptError(ErrorCode::MissingExplicitType,
                       fmt::format("type cannot be inferred and wasn't specified: {}", name), loc);
}

inline ScriptError missing_type_arg(const SourceLocation& loc) {
    return ScriptError(ErrorCode::MissingTypeArg, "generic function requires type argument", loc);
}

inline ScriptError unexpected_type_arg(const SourceLocation& loc) {
    return ScriptError(ErrorCode::UnexpectedTypeArg, "unexpected type argument", loc);
}

inline ScriptError unexpected_generic_function(string_view fn_desc) {
    return ScriptError(ErrorCode::UnexpectedGenericFunction,
                       fmt::format("unexpected generic function: {}", fn_desc));

}
inline ScriptError unexpected_generic_function(string_view fn_desc, const SourceLocation& loc) {
    return ScriptError(ErrorCode::UnexpectedGenericFunction,
                       fmt::format("unexpected generic function: {}", fn_desc), loc);
}

inline ScriptError function_not_found(string_view fn_desc,
                                      string_view candidates, const SourceLocation& loc) {
    return ScriptError(ErrorCode::FunctionNotFound,
                       fmt::format("function not found: {}\n"
                                   "   Candidates:\n{}", fn_desc, candidates), loc);
}

inline ScriptError function_conflict(string_view fn_desc,
                                     string_view candidates, const SourceLocation& loc) {
    return ScriptError(ErrorCode::FunctionConflict,
                       fmt::format("function cannot be uniquely resolved: {}\n"
                                   "   Candidates:\n{}", fn_desc, candidates), loc);
}

inline ScriptError function_not_found_in_class(string_view fn, string_view cls) {
    return ScriptError(ErrorCode::FunctionNotFoundInClass,
                       fmt::format("instance function '{}' not found in class '{}'", fn, cls));
}

inline ScriptError too_many_locals() {
    return ScriptError(ErrorCode::TooManyLocals,
                       fmt::format("too many local values in function"));
}

inline ScriptError condition_not_bool() {
    return ScriptError(ErrorCode::ConditionNotBool, "condition doesn't evaluate to Bool");
}

ScriptError declaration_type_mismatch(const TypeInfo& decl, const TypeInfo& now,
                                      const SourceLocation& loc);

ScriptError definition_type_mismatch(const TypeInfo& exp, const TypeInfo& got,
                                     const SourceLocation& loc);

ScriptError definition_param_type_mismatch(size_t idx, const TypeInfo& exp, const TypeInfo& got,
                                           const SourceLocation& loc);

ScriptError branch_type_mismatch(const TypeInfo& exp, const TypeInfo& got);

ScriptError list_type_mismatch(const TypeInfo& got, const SourceLocation& loc);

ScriptError list_elem_type_mismatch(const TypeInfo& exp, const TypeInfo& got,
                                    const SourceLocation& loc);

ScriptError struct_type_mismatch(const TypeInfo& got, const SourceLocation& loc);

ScriptError struct_unknown_key(const TypeInfo& struct_type, const std::string& key,
                               const SourceLocation& loc);

inline ScriptError struct_duplicate_key(const std::string& key, const SourceLocation& loc) {
    return ScriptError(ErrorCode::StructDuplicateKey,
                       fmt::format("duplicate struct key \"{}\"", key), loc);
}
ScriptError struct_key_type_mismatch(const TypeInfo& struct_type, const TypeInfo& spec,
                                     const TypeInfo& got, const SourceLocation& loc);


inline ScriptError intrinsics_function_error(std::string message, const SourceLocation& loc) {
    return ScriptError(ErrorCode::IntrinsicsFunctionError, std::move(message), loc);
}


inline ScriptError unresolved_symbol(string_view name) {
    return ScriptError(ErrorCode::UnresolvedSymbol,
                       fmt::format("unresolved symbol: {}", name));
}


inline ScriptError import_error(string_view name) {
    return ScriptError(ErrorCode::ImportError,
                       fmt::format("module cannot be imported: {}", name));
}


} // namespace xci::script

template <> struct fmt::formatter<xci::script::ErrorCode> : ostream_formatter {};

#endif // include guard
