// Code.cpp created on 2019-05-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Code.h"
#include <xci/data/coding/leb128.h>
#include <xci/compat/macros.h>


namespace xci::script {

using xci::data::leb128_encode;


std::ostream& operator<<(std::ostream& os, Opcode v)
{
    switch (v) {
        case Opcode::Noop:              return os << "NOOP";
        case Opcode::LogicalNot:        return os << "LOGICAL_NOT";
        case Opcode::LogicalOr:         return os << "LOGICAL_OR";
        case Opcode::LogicalAnd:        return os << "LOGICAL_AND";
        case Opcode::Equal:             return os << "EQUAL";
        case Opcode::NotEqual:          return os << "NOT_EQUAL";
        case Opcode::LessEqual:         return os << "LESS_EQUAL";
        case Opcode::GreaterEqual:      return os << "GREATER_EQUAL";
        case Opcode::LessThan:          return os << "LESS_THAN";
        case Opcode::GreaterThan:       return os << "GREATER_THAN";
        case Opcode::BitwiseNot_8:
        case Opcode::BitwiseNot_32:
        case Opcode::BitwiseNot_64:     return os << "BITWISE_NOT";
        case Opcode::BitwiseOr_8:
        case Opcode::BitwiseOr_32:
        case Opcode::BitwiseOr_64:      return os << "BITWISE_OR";
        case Opcode::BitwiseAnd_8:
        case Opcode::BitwiseAnd_32:
        case Opcode::BitwiseAnd_64:     return os << "BITWISE_AND";
        case Opcode::BitwiseXor_8:
        case Opcode::BitwiseXor_32:
        case Opcode::BitwiseXor_64:     return os << "BITWISE_XOR";
        case Opcode::ShiftLeft:         return os << "SHIFT_LEFT";
        case Opcode::ShiftRight:        return os << "SHIFT_RIGHT";
        case Opcode::Neg:               return os << "NEG";
        case Opcode::Add:               return os << "ADD";
        case Opcode::Sub:               return os << "SUB";
        case Opcode::Mul:               return os << "MUL";
        case Opcode::Div:               return os << "DIV";
        case Opcode::Mod:               return os << "MOD";
        case Opcode::Exp:               return os << "EXP";
        case Opcode::ListSubscript:     return os << "LIST_SUBSCRIPT";
        case Opcode::ListLength:        return os << "LIST_LENGTH";
        case Opcode::ListSlice:         return os << "LIST_SLICE";
        case Opcode::ListConcat:        return os << "LIST_CONCAT";
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
        case Opcode::Annotation:        return os << "(ANNOTATION)";
    }
    XCI_UNREACHABLE;
}


size_t Code::add_L1(Opcode opcode, size_t arg)
{
    const auto orig_ops = m_ops.size();
    add_opcode(opcode);
    leb128_encode(m_ops, arg);
    return m_ops.size() - orig_ops;
}


size_t Code::add_L2(Opcode opcode, size_t arg1, size_t arg2)
{
    const auto orig_ops = m_ops.size();
    add_opcode(opcode);
    leb128_encode(m_ops, arg1);
    leb128_encode(m_ops, arg2);
    return m_ops.size() - orig_ops;
}


} // namespace xci::script
