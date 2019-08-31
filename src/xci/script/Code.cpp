// Code.cpp created on 2019-05-23, part of XCI toolkit
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

#include "Code.h"
#include <xci/compat/macros.h>


namespace xci::script {


std::ostream& operator<<(std::ostream& os, Opcode v)
{
    switch (v) {
        case Opcode::Noop:          return os << "NOOP";
        case Opcode::LogicalOr:     return os << "LOGICAL_OR";
        case Opcode::LogicalAnd:    return os << "LOGICAL_AND";
        case Opcode::Equal_8:
        case Opcode::Equal_32:
        case Opcode::Equal_64:      return os << "EQUAL";
        case Opcode::NotEqual_8:
        case Opcode::NotEqual_32:
        case Opcode::NotEqual_64:   return os << "NOT_EQUAL";
        case Opcode::LessEqual_8:
        case Opcode::LessEqual_32:
        case Opcode::LessEqual_64:  return os << "LESS_EQUAL";
        case Opcode::GreaterEqual_8:
        case Opcode::GreaterEqual_32:
        case Opcode::GreaterEqual_64: return os << "GREATER_EQUAL";
        case Opcode::LessThan_8:
        case Opcode::LessThan_32:
        case Opcode::LessThan_64:   return os << "LESS_THAN";
        case Opcode::GreaterThan_8:
        case Opcode::GreaterThan_32:
        case Opcode::GreaterThan_64: return os << "GREATER_THAN";
        case Opcode::BitwiseOr_8:
        case Opcode::BitwiseOr_32:
        case Opcode::BitwiseOr_64:  return os << "BITWISE_OR";
        case Opcode::BitwiseAnd_8:
        case Opcode::BitwiseAnd_32:
        case Opcode::BitwiseAnd_64: return os << "BITWISE_AND";
        case Opcode::BitwiseXor_8:
        case Opcode::BitwiseXor_32:
        case Opcode::BitwiseXor_64: return os << "BITWISE_XOR";
        case Opcode::ShiftLeft_8:
        case Opcode::ShiftLeft_32:
        case Opcode::ShiftLeft_64:  return os << "SHIFT_LEFT";
        case Opcode::ShiftRight_8:
        case Opcode::ShiftRight_32:
        case Opcode::ShiftRight_64: return os << "SHIFT_RIGHT";
        case Opcode::Add_8:
        case Opcode::Add_32:
        case Opcode::Add_64:        return os << "ADD";
        case Opcode::Sub_8:
        case Opcode::Sub_32:
        case Opcode::Sub_64:        return os << "SUB";
        case Opcode::Mul_8:
        case Opcode::Mul_32:
        case Opcode::Mul_64:        return os << "MUL";
        case Opcode::Div_8:
        case Opcode::Div_32:
        case Opcode::Div_64:        return os << "DIV";
        case Opcode::Mod_8:
        case Opcode::Mod_32:
        case Opcode::Mod_64:        return os << "MOD";
        case Opcode::Exp_8:
        case Opcode::Exp_32:
        case Opcode::Exp_64:        return os << "EXP";
        case Opcode::LogicalNot:    return os << "LOGICAL_NOT";
        case Opcode::BitwiseNot_8:
        case Opcode::BitwiseNot_32:
        case Opcode::BitwiseNot_64: return os << "BITWISE_NOT";
        case Opcode::Neg_8:
        case Opcode::Neg_32:
        case Opcode::Neg_64:        return os << "NEG";
        case Opcode::Invoke:        return os << "INVOKE";
        case Opcode::Execute:       return os << "EXECUTE";
        case Opcode::LoadStatic:    return os << "LOAD_STATIC";
        case Opcode::LoadModule:    return os << "LOAD_MODULE";
        case Opcode::LoadFunction:  return os << "LOAD_FUNCTION";
        case Opcode::CopyVariable:  return os << "COPY_VARIABLE";
        case Opcode::CopyArgument:  return os << "COPY_ARGUMENT";
        case Opcode::Drop:          return os << "DROP";
        case Opcode::Call0:         return os << "CALL0";
        case Opcode::Call1:         return os << "CALL1";
        case Opcode::Call:          return os << "CALL";
        case Opcode::MakeClosure:   return os << "MAKE_CLOSURE";
        case Opcode::IncRef:        return os << "INC_REF";
        case Opcode::DecRef:        return os << "DEC_REF";
        case Opcode::Jump:          return os << "JUMP";
        case Opcode::JumpIfNot:     return os << "JUMP_IF_NOT";
    }
    UNREACHABLE;
}


} // namespace xci::script
