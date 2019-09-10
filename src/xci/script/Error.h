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

#ifndef XCIKIT_ERROR_H
#define XCIKIT_ERROR_H

#include "Value.h"
#include <xci/core/format.h>

namespace xci::script {


struct SourceInfo {
    size_t line_number = 0;
    size_t byte_in_line = 0;
    const char* line_begin = nullptr;
    const char* line_end = nullptr;
    const char* source = nullptr;

    template <typename Input, typename Position>
    void load(const Input &in, const Position& pos) {
        line_begin = in.begin_of_line(pos);
        line_end = in.end_of_line(pos);
        line_number = pos.line;
        byte_in_line = pos.byte_in_line;
        source = in.source();
    }
};


class Error : public std::exception {
public:
    explicit Error(std::string msg) : m_msg(std::move(msg)) {}
    explicit Error(std::string msg, const SourceInfo& si) :
        m_msg(std::move(msg)),
        m_file(core::format("{}:{}:{}",
            si.source, si.line_number, si.byte_in_line)),
        m_detail(core::format("{}\n{}^",
            std::string{si.line_begin, si.line_end},
            std::string(si.byte_in_line, ' ')))
    {}

    const char* what() const noexcept override { return m_msg.c_str(); }
    const std::string& detail() const noexcept { return m_detail; }
    const std::string& file() const noexcept { return m_file; }

private:
    std::string m_msg;
    std::string m_file;
    std::string m_detail;
};


inline std::ostream& operator<<(std::ostream& os, const Error& e) noexcept
{
    if (!e.file().empty())
        os << e.file() << ": ";
    os << "Error: " << e.what();
    if (!e.detail().empty())
        os << std::endl << e.detail();
    return os;
}


struct NotImplemented : public Error {
    explicit NotImplemented(const std::string& name)
        : Error(core::format("not implemented: {}", name)) {}
};


struct BadInstruction : public Error {
    explicit BadInstruction(uint8_t code)
            : Error(core::format("bad instruction: {}", code)) {}
};


struct ParseError : public Error {
    explicit ParseError(const std::string& msg)
            : Error(core::format("parse error: {}", msg)) {}
};


struct StackUnderflow : public Error {
    explicit StackUnderflow() : Error(core::format("stack underflow")) {}
};


struct StackOverflow : public Error {
    explicit StackOverflow() : Error(core::format("stack overflow")) {}
};


struct UndefinedName : public Error {
    explicit UndefinedName(const std::string& name, const SourceInfo& si)
        : Error(core::format("undefined name: {}", name), si) {}
};


struct MultipleDeclarationError : public Error {
    explicit MultipleDeclarationError(const std::string& name)
            : Error(core::format("multiple declaration of name: {}", name)) {}
};


struct UnexpectedArgument : public Error {
    explicit UnexpectedArgument(int idx, const SourceInfo& si)
            : Error(core::format("unexpected argument #{}", idx), si) {}
};


struct UnsupportedOperandsError : public Error {
    explicit UnsupportedOperandsError(const std::string& op)
            : Error(core::format("unsupported operands to '{}'", op)) {}
};


struct UnexpectedArgumentCount : public Error {
    explicit UnexpectedArgumentCount(int exp, int got)
            : Error(core::format("function expects {} args, called with {} args",
                    exp, got)) {}
};


struct UnknownTypeName : public Error {
    explicit UnknownTypeName(const std::string& name)
        : Error(core::format("unknown type name: {}", name)) {}
};


struct UnexpectedArgumentType : public Error {
    explicit UnexpectedArgumentType(int idx, const TypeInfo& exp, const TypeInfo& got, const SourceInfo& si)
            : Error(core::format("function expects {} for arg #{}, called with {}",
                                 exp, idx, got), si) {}
};


struct UnexpectedReturnType : public Error {
    explicit UnexpectedReturnType(const TypeInfo& exp, const TypeInfo& got)
            : Error(core::format("function returns {}, body evaluates to {}",
                                 exp, got)) {}
};


struct MissingExplicitType : public Error {
    explicit MissingExplicitType()
        : Error("type cannot be inferred and wasn't specified") {}
};


struct FunctionNotFound : public Error {
    explicit FunctionNotFound(const std::string& name, const std::string& args,
                              const std::string& candidates)
        : Error(core::format("function not found: {} {}\n   Candidates:\n{}", name, args, candidates)) {}
};


struct TooManyLocalsError : public Error {
    explicit TooManyLocalsError()
            : Error(core::format("too many local values in function")) {}
};


struct ConditionNotBool : public Error {
    explicit ConditionNotBool() : Error("condition doesn't evaluate to Bool") {}
};


struct DefinitionTypeMismatch : public Error {
    explicit DefinitionTypeMismatch(const TypeInfo& exp, const TypeInfo& got)
            : Error(core::format("definition type mismatch: specified {}, inferred {}",
                                 exp, got)) {}
};


struct DefinitionParamTypeMismatch : public Error {
    explicit DefinitionParamTypeMismatch(int idx, const TypeInfo& exp, const TypeInfo& got)
            : Error(core::format("definition type mismatch: specified {} for param #{}, inferred {}",
                                 exp, idx, got)) {}
};


struct BranchTypeMismatch : public Error {
    explicit BranchTypeMismatch(const TypeInfo& exp, const TypeInfo& got)
            : Error(core::format("branch type mismatch: then branch {} else branch {}",
                                 exp, got)) {}
};


struct ListElemTypeMismatch : public Error {
    explicit ListElemTypeMismatch(const TypeInfo& exp, const TypeInfo& got)
            : Error(core::format("list element type mismatch: got {} in list of {}",
                                 got, exp)) {}
};


struct IndexOutOfBounds : public Error {
    explicit IndexOutOfBounds(int idx, size_t len)
            : Error(core::format("list index out of bounds: {} not in [0..{}]",
                                 idx, len-1)) {}
};


struct IntrinsicsFunctionError : public Error {
    explicit IntrinsicsFunctionError(const std::string& message)
        : Error("intrinsics function: " + message) {}
};


} // namespace xci::script

#endif // include guard
