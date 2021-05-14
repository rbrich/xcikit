// Code.cpp created on 2019-05-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Code.h"
#include <xci/data/coding/leb128.h>
#include <xci/compat/macros.h>


namespace xci::script {

using xci::data::encode_leb128;


std::ostream& operator<<(std::ostream& os, Opcode v)
{
    switch (v) {
        case Opcode::Noop:              return os << "NOOP";
        case Opcode::LogicalOr:         return os << "LOGICAL_OR";
        case Opcode::LogicalAnd:        return os << "LOGICAL_AND";
        case Opcode::Equal_8:
        case Opcode::Equal_32:
        case Opcode::Equal_64:          return os << "EQUAL";
        case Opcode::NotEqual_8:
        case Opcode::NotEqual_32:
        case Opcode::NotEqual_64:       return os << "NOT_EQUAL";
        case Opcode::LessEqual_8:
        case Opcode::LessEqual_32:
        case Opcode::LessEqual_64:      return os << "LESS_EQUAL";
        case Opcode::GreaterEqual_8:
        case Opcode::GreaterEqual_32:
        case Opcode::GreaterEqual_64:   return os << "GREATER_EQUAL";
        case Opcode::LessThan_8:
        case Opcode::LessThan_32:
        case Opcode::LessThan_64:       return os << "LESS_THAN";
        case Opcode::GreaterThan_8:
        case Opcode::GreaterThan_32:
        case Opcode::GreaterThan_64:    return os << "GREATER_THAN";
        case Opcode::BitwiseOr_8:
        case Opcode::BitwiseOr_32:
        case Opcode::BitwiseOr_64:      return os << "BITWISE_OR";
        case Opcode::BitwiseAnd_8:
        case Opcode::BitwiseAnd_32:
        case Opcode::BitwiseAnd_64:     return os << "BITWISE_AND";
        case Opcode::BitwiseXor_8:
        case Opcode::BitwiseXor_32:
        case Opcode::BitwiseXor_64:     return os << "BITWISE_XOR";
        case Opcode::ShiftLeft_8:
        case Opcode::ShiftLeft_32:
        case Opcode::ShiftLeft_64:      return os << "SHIFT_LEFT";
        case Opcode::ShiftRight_8:
        case Opcode::ShiftRight_32:
        case Opcode::ShiftRight_64:     return os << "SHIFT_RIGHT";
        case Opcode::Add_8:
        case Opcode::Add_32:
        case Opcode::Add_64:            return os << "ADD";
        case Opcode::Sub_8:
        case Opcode::Sub_32:
        case Opcode::Sub_64:            return os << "SUB";
        case Opcode::Mul_8:
        case Opcode::Mul_32:
        case Opcode::Mul_64:            return os << "MUL";
        case Opcode::Div_8:
        case Opcode::Div_32:
        case Opcode::Div_64:            return os << "DIV";
        case Opcode::Mod_8:
        case Opcode::Mod_32:
        case Opcode::Mod_64:            return os << "MOD";
        case Opcode::Exp_8:
        case Opcode::Exp_32:
        case Opcode::Exp_64:            return os << "EXP";
        case Opcode::LogicalNot:        return os << "LOGICAL_NOT";
        case Opcode::BitwiseNot_8:
        case Opcode::BitwiseNot_32:
        case Opcode::BitwiseNot_64:     return os << "BITWISE_NOT";
        case Opcode::Neg_8:
        case Opcode::Neg_32:
        case Opcode::Neg_64:            return os << "NEG";
        case Opcode::Subscript:         return os << "SUBSCRIPT";
        case Opcode::Invoke:            return os << "INVOKE";
        case Opcode::LoadStatic:        return os << "LOAD_STATIC";
        case Opcode::LoadModule:        return os << "LOAD_MODULE";
        case Opcode::LoadFunction:      return os << "LOAD_FUNCTION";
        case Opcode::Cast:              return os << "CAST";
        case Opcode::Copy:              return os << "COPY";
        case Opcode::Drop:              return os << "DROP";
        case Opcode::Swap:              return os << "SWAP";
        case Opcode::Call0:             return os << "CALL0";
        case Opcode::Call1:             return os << "CALL1";
        case Opcode::Call:              return os << "CALL";
        case Opcode::Execute:           return os << "EXECUTE";
        case Opcode::MakeClosure:       return os << "MAKE_CLOSURE";
        case Opcode::MakeList:          return os << "MAKE_LIST";
        case Opcode::SetBase:           return os << "SET_BASE";
        case Opcode::IncRef:            return os << "INC_REF";
        case Opcode::DecRef:            return os << "DEC_REF";
        case Opcode::Jump:              return os << "JUMP";
        case Opcode::JumpIfNot:         return os << "JUMP_IF_NOT";
    }
    UNREACHABLE;
}


size_t Code::add_L1(Opcode opcode, size_t arg)
{
    const auto orig_ops = m_ops.size();
    add_opcode(opcode);
    encode_leb128(m_ops, arg);
    return m_ops.size() - orig_ops;
}


size_t Code::add_L2(Opcode opcode, size_t arg1, size_t arg2)
{
    const auto orig_ops = m_ops.size();
    add_opcode(opcode);
    encode_leb128(m_ops, arg1);
    encode_leb128(m_ops, arg2);
    return m_ops.size() - orig_ops;
}


} // namespace xci::script
