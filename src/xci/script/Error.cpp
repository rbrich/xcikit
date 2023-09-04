// Error.cpp created on 2022-05-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Error.h"
#include "dump.h"
#include <xci/compat/macros.h>

namespace xci::script {


std::ostream& operator<<(std::ostream& os, ErrorCode v)
{
    switch (v) {
        case ErrorCode::NotImplemented:             return os << "NotImplemented";
        case ErrorCode::ValueOutOfRange:            return os << "ValueOutOfRange";
        case ErrorCode::IndexOutOfBounds:           return os << "IndexOutOfBounds";
        case ErrorCode::BadInstruction:             return os << "BadInstruction";
        case ErrorCode::ParseError:                 return os << "ParseError";
        case ErrorCode::StackUnderflow:             return os << "StackUnderflow";
        case ErrorCode::StackOverflow:              return os << "StackOverflow";
        case ErrorCode::UndefinedName:              return os << "UndefinedName";
        case ErrorCode::UndefinedTypeName:          return os << "UndefinedTypeName";
        case ErrorCode::RedefinedName:              return os << "RedefinedName";
        case ErrorCode::UnsupportedOperandsError:   return os << "UnsupportedOperandsError";
        case ErrorCode::UnknownTypeName:            return os << "UnknownTypeName";
        case ErrorCode::UnexpectedArgument:         return os << "UnexpectedArgument";
        case ErrorCode::UnexpectedArgumentType:     return os << "UnexpectedArgumentType";
        case ErrorCode::UnexpectedReturnType:       return os << "UnexpectedReturnType";
        case ErrorCode::MissingExplicitType:        return os << "MissingExplicitType";
        case ErrorCode::MissingTypeArg:             return os << "MissingTypeArg";
        case ErrorCode::UnexpectedTypeArg:          return os << "UnexpectedTypeArg";
        case ErrorCode::UnexpectedGenericFunction:  return os << "UnexpectedGenericFunction";
        case ErrorCode::FunctionNotFound:           return os << "FunctionNotFound";
        case ErrorCode::FunctionConflict:           return os << "FunctionConflict";
        case ErrorCode::FunctionNotFoundInClass:    return os << "FunctionNotFoundInClass";
        case ErrorCode::TooManyLocals:              return os << "TooManyLocals";
        case ErrorCode::ConditionNotBool:           return os << "ConditionNotBool";
        case ErrorCode::DeclarationTypeMismatch:    return os << "DeclarationTypeMismatch";
        case ErrorCode::DefinitionTypeMismatch:     return os << "DefinitionTypeMismatch";
        case ErrorCode::DefinitionParamTypeMismatch:return os << "DefinitionParamTypeMismatch";
        case ErrorCode::BranchTypeMismatch:         return os << "BranchTypeMismatch";
        case ErrorCode::ListTypeMismatch:           return os << "ListTypeMismatch";
        case ErrorCode::ListElemTypeMismatch:       return os << "ListElemTypeMismatch";
        case ErrorCode::StructTypeMismatch:         return os << "StructTypeMismatch";
        case ErrorCode::StructUnknownKey:           return os << "StructUnknownKey";
        case ErrorCode::StructDuplicateKey:         return os << "StructDuplicateKey";
        case ErrorCode::StructKeyTypeMismatch:      return os << "StructKeyTypeMismatch";
        case ErrorCode::IntrinsicsFunctionError:    return os << "IntrinsicsFunctionError";
        case ErrorCode::UnresolvedSymbol:           return os << "UnresolvedSymbol";
        case ErrorCode::ImportError:                return os << "ImportError";
        case ErrorCode::ModuleNotFound:             return os << "ModuleNotFound";
    }
    XCI_UNREACHABLE;
}


ScriptError unexpected_argument(const TypeInfo& ftype, const SourceLocation& loc) {
    return ScriptError(ErrorCode::UnexpectedArgument, fmt::format("unexpected argument for called type {}", ftype), loc);
}


ScriptError unexpected_argument_type(const TypeInfo& exp, const TypeInfo& got,
                                     const SourceLocation& loc) {
    return ScriptError(ErrorCode::UnexpectedArgumentType,
                       fmt::format("function expects {}, called with {}", exp, got), loc);
}

ScriptError unexpected_argument_type(const TypeInfo& exp, const TypeInfo& got,
                                     const TypeInfo& exp_arg, const TypeInfo& got_arg,
                                     const SourceLocation& loc) {
    return ScriptError(ErrorCode::UnexpectedArgumentType,
                       fmt::format("function expects {} in {}, called with {} in {}",
                                   exp, exp_arg, got, got_arg), loc);
}


ScriptError unexpected_return_type(const TypeInfo& exp, const TypeInfo& got, const SourceLocation& loc) {
    return ScriptError(ErrorCode::UnexpectedReturnType,
                       fmt::format("function returns {}, body evaluates to {}",
                                   exp, got), loc);
}


ScriptError declaration_type_mismatch(const TypeInfo& decl, const TypeInfo& now,
                                      const SourceLocation& loc) {
    return ScriptError(ErrorCode::DeclarationTypeMismatch,
                       fmt::format("declared type mismatch: previous {}, this {}",
                                   decl, now),
                       loc);
}


ScriptError definition_type_mismatch(const TypeInfo& exp, const TypeInfo& got,
                                     const SourceLocation& loc) {
    return ScriptError(ErrorCode::DefinitionTypeMismatch,
                       fmt::format("definition type mismatch: specified {}, inferred {}",
                                   exp, got),
                       loc);
}


ScriptError definition_param_type_mismatch(size_t idx, const TypeInfo& exp, const TypeInfo& got,
                                           const SourceLocation& loc) {
    return ScriptError(ErrorCode::DefinitionParamTypeMismatch,
                       fmt::format("definition type mismatch: specified {} for param #{}, inferred {}",
                                   exp, idx, got),
                       loc);
}


ScriptError branch_type_mismatch(const TypeInfo& exp, const TypeInfo& got) {
    return ScriptError(ErrorCode::BranchTypeMismatch,
                       fmt::format("branch type mismatch: expected {}, got {}",
                                   exp, got));
}


ScriptError list_type_mismatch(const TypeInfo& got, const SourceLocation& loc) {
    return ScriptError(ErrorCode::ListTypeMismatch,
                       fmt::format("cannot cast a list to {}", got),
                       loc);
}


ScriptError list_elem_type_mismatch(const TypeInfo& exp, const TypeInfo& got,
                                    const SourceLocation& loc) {
    return ScriptError(ErrorCode::ListElemTypeMismatch,
                       fmt::format("list element type mismatch: got {} in list of {}", got, exp),
                       loc);
}


ScriptError struct_type_mismatch(const TypeInfo& got, const SourceLocation& loc) {
    return ScriptError(ErrorCode::StructTypeMismatch,
                       fmt::format("cannot cast a struct initializer to {}", got),
                       loc);
}


ScriptError struct_unknown_key(const TypeInfo& struct_type, const std::string& key,
                               const SourceLocation& loc) {
    return ScriptError(ErrorCode::StructUnknownKey,
                       fmt::format("struct key \"{}\" doesn't match struct type {}",
                                   key, struct_type),
                       loc);
}


ScriptError struct_key_type_mismatch(const TypeInfo& struct_type,
                                     const TypeInfo& spec, const TypeInfo& got,
                                     const SourceLocation& loc) {
    return ScriptError(ErrorCode::StructKeyTypeMismatch,
                       fmt::format("struct item type mismatch: specified {} in {}, inferred {}",
                                   spec, struct_type, got),
                       loc);
}


} // namespace xci::script
