// Error.h created on 2019-05-18, part of XCI toolkit
// Copyright 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef XCI_SCRIPT_ERROR_H
#define XCI_SCRIPT_ERROR_H

#include "Value.h"
#include "SourceInfo.h"
#include "dump.h"
#include <xci/core/error.h>
#include <xci/core/format.h>

namespace xci::script {


class ScriptError : public core::Error {
public:
    explicit ScriptError(std::string msg) : Error(std::move(msg)) {}
    explicit ScriptError(std::string msg, const SourceInfo& si) :
        Error(std::move(msg)),
        m_file(core::format("{}:{}:{}",
            si.source, si.line_number, si.byte_in_line)),
        m_detail(core::format("{}\n{}^",
            std::string{si.line_begin, si.line_end},
            std::string(si.byte_in_line, ' ')))
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
        : ScriptError(core::format("not implemented: {}", name)) {}
};


struct BadInstruction : public ScriptError {
    explicit BadInstruction(uint8_t code)
            : ScriptError(core::format("bad instruction: {}", code)) {}
};


struct ParseError : public ScriptError {
    explicit ParseError(const std::string& msg)
            : ScriptError(core::format("parse error: {}", msg)) {}
};


struct StackUnderflow : public ScriptError {
    explicit StackUnderflow() : ScriptError(core::format("stack underflow")) {}
};


struct StackOverflow : public ScriptError {
    explicit StackOverflow() : ScriptError(core::format("stack overflow")) {}
};


struct UndefinedName : public ScriptError {
    explicit UndefinedName(const std::string& name, const SourceInfo& si)
        : ScriptError(core::format("undefined name: {}", name), si) {}
};


struct UndefinedTypeName : public ScriptError {
    explicit UndefinedTypeName(const std::string& name)
            : ScriptError(core::format("undefined type name: {}", name)) {}
};


struct MultipleDeclarationError : public ScriptError {
    explicit MultipleDeclarationError(const std::string& name)
            : ScriptError(core::format("multiple declaration of name: {}", name)) {}
};


struct UnexpectedArgument : public ScriptError {
    explicit UnexpectedArgument(int idx, const SourceInfo& si)
            : ScriptError(core::format("unexpected argument #{}", idx), si) {}
};


struct UnsupportedOperandsError : public ScriptError {
    explicit UnsupportedOperandsError(const std::string& op)
            : ScriptError(core::format("unsupported operands to '{}'", op)) {}
};


struct UnexpectedArgumentCount : public ScriptError {
    explicit UnexpectedArgumentCount(int exp, int got)
            : ScriptError(core::format("function expects {} args, called with {} args",
                    exp, got)) {}
};


struct UnknownTypeName : public ScriptError {
    explicit UnknownTypeName(const std::string& name)
        : ScriptError(core::format("unknown type name: {}", name)) {}
};


struct UnexpectedArgumentType : public ScriptError {
    explicit UnexpectedArgumentType(int idx, const TypeInfo& exp, const TypeInfo& got, const SourceInfo& si)
            : ScriptError(core::format("function expects {} for arg #{}, called with {}",
                                 exp, idx, got), si) {}
};


struct UnexpectedReturnType : public ScriptError {
    explicit UnexpectedReturnType(const TypeInfo& exp, const TypeInfo& got)
            : ScriptError(core::format("function returns {}, body evaluates to {}",
                                 exp, got)) {}
};


struct MissingExplicitType : public ScriptError {
    explicit MissingExplicitType()
        : ScriptError("type cannot be inferred and wasn't specified") {}
};


struct FunctionNotFound : public ScriptError {
    explicit FunctionNotFound(const std::string& name, const std::string& args,
                              const std::string& candidates)
        : ScriptError(core::format("function not found: {} {}\n   Candidates:\n{}", name, args, candidates)) {}
};


struct FunctionConflict : public ScriptError {
    explicit FunctionConflict(const std::string& name, const std::string& args,
                              const std::string& candidates)
            : ScriptError(core::format("function cannot be uniquely resolved: {} {}\n   Candidates:\n{}", name, args, candidates)) {}
};


struct FunctionNotFoundInClass : public ScriptError {
    explicit FunctionNotFoundInClass(const std::string& fn, const std::string& cls)
            : ScriptError(core::format("instance function '{}' not found in class '{}'",
                                 fn, cls)) {}
};


struct TooManyLocalsError : public ScriptError {
    explicit TooManyLocalsError()
            : ScriptError(core::format("too many local values in function")) {}
};


struct ConditionNotBool : public ScriptError {
    explicit ConditionNotBool() : ScriptError("condition doesn't evaluate to Bool") {}
};


struct DefinitionTypeMismatch : public ScriptError {
    explicit DefinitionTypeMismatch(const TypeInfo& exp, const TypeInfo& got)
            : ScriptError(core::format("definition type mismatch: specified {}, inferred {}",
                                 exp, got)) {}
};


struct DefinitionParamTypeMismatch : public ScriptError {
    explicit DefinitionParamTypeMismatch(int idx, const TypeInfo& exp, const TypeInfo& got)
            : ScriptError(core::format("definition type mismatch: specified {} for param #{}, inferred {}",
                                 exp, idx, got)) {}
};


struct BranchTypeMismatch : public ScriptError {
    explicit BranchTypeMismatch(const TypeInfo& exp, const TypeInfo& got)
            : ScriptError(core::format("branch type mismatch: then branch {} else branch {}",
                                 exp, got)) {}
};


struct ListElemTypeMismatch : public ScriptError {
    explicit ListElemTypeMismatch(const TypeInfo& exp, const TypeInfo& got)
            : ScriptError(core::format("list element type mismatch: got {} in list of {}",
                                 got, exp)) {}
};


struct IndexOutOfBounds : public ScriptError {
    explicit IndexOutOfBounds(int idx, size_t len)
            : ScriptError(core::format("list index out of bounds: {} not in [0..{}]",
                                 idx, len-1)) {}
};


struct IntrinsicsFunctionError : public ScriptError {
    explicit IntrinsicsFunctionError(const std::string& message)
        : ScriptError("intrinsics function: " + message) {}
};


} // namespace xci::script

#endif // include guard
