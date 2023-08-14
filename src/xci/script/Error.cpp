// Error.cpp created on 2022-05-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Error.h"
#include "dump.h"

namespace xci::script {


UnexpectedArgument::UnexpectedArgument(const TypeInfo& ftype, const SourceLocation& loc)
        : ScriptError(fmt::format("unexpected argument for called type {}", ftype), loc)
{}


UnexpectedArgumentType::UnexpectedArgumentType(const TypeInfo& exp, const TypeInfo& got,
                                               const SourceLocation& loc)
        : ScriptError(fmt::format("function expects {}, called with {}", exp, got), loc)
{}


UnexpectedArgumentType::UnexpectedArgumentType(const TypeInfo& exp, const TypeInfo& got,
                                               const TypeInfo& exp_arg, const TypeInfo& got_arg,
                                               const SourceLocation& loc)
        : ScriptError(fmt::format("function expects {} in {}, called with {} in {}",
                                  exp, exp_arg, got, got_arg), loc)
{}


UnexpectedReturnType::UnexpectedReturnType(const TypeInfo& exp, const TypeInfo& got, const SourceLocation& loc)
        : ScriptError(fmt::format("function returns {}, body evaluates to {}",
                                  exp, got), loc)
{}


DeclarationTypeMismatch::DeclarationTypeMismatch(const TypeInfo& decl, const TypeInfo& now,
                                                 const SourceLocation& loc)
        : ScriptError(fmt::format("declared type mismatch: previous {}, this {}",
                                  decl, now),
                      loc)
{}


DefinitionTypeMismatch::DefinitionTypeMismatch(const TypeInfo& exp, const TypeInfo& got,
                                               const SourceLocation& loc)
        : ScriptError(fmt::format("definition type mismatch: specified {}, inferred {}",
                                  exp, got),
                      loc)
{}


DefinitionParamTypeMismatch::DefinitionParamTypeMismatch(size_t idx, const TypeInfo& exp, const TypeInfo& got, const SourceLocation& loc)
        : ScriptError(fmt::format("definition type mismatch: specified {} for param #{}, inferred {}",
                                  exp, idx, got),
                      loc)
{}


BranchTypeMismatch::BranchTypeMismatch(const TypeInfo& exp, const TypeInfo& got)
        : ScriptError(fmt::format("branch type mismatch: expected {}, got {}",
                                  exp, got))
{}


ListElemTypeMismatch::ListElemTypeMismatch(const TypeInfo& exp, const TypeInfo& got,
                                           const SourceLocation& loc)
        : ScriptError(fmt::format("list element type mismatch: got {} in list of {}",
                                  got, exp),
                      loc)
{}


StructTypeMismatch::StructTypeMismatch(const TypeInfo& got, const SourceLocation& loc)
        : ScriptError(fmt::format("cannot cast a struct initializer to {}",
                                  got),
                      loc)
{}


StructUnknownKey::StructUnknownKey(const TypeInfo& struct_type, const std::string& key,
                                   const SourceLocation& loc)
        : ScriptError(fmt::format("struct key \"{}\" doesn't match struct type {}",
                                  key, struct_type),
                      loc)
{}


StructKeyTypeMismatch::StructKeyTypeMismatch(const TypeInfo& struct_type, const TypeInfo& spec, const TypeInfo& got,
                                             const SourceLocation& loc)
        : ScriptError(fmt::format("struct item type mismatch: specified {} in {}, inferred {}",
                                  spec, struct_type, got),
                      loc)
{}


} // namespace xci::script
