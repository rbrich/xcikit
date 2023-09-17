// Machine.cpp created on 2019-05-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Machine.h"
#include "Builtin.h"
#include "Value.h"
#include "Error.h"
#include "dump.h"
#include "typing/type_index.h"
#include <xci/data/coding/leb128.h>

#include <fmt/core.h>
#include <cassert>
#include <cstddef>  // std::ptrdiff_t

namespace xci::script {

using xci::data::leb128_decode;
using fmt::format;

#ifdef XCI_SCRIPT_COMPUTED_GOTO
    #define OP_LOOP         op_loop:
    #define OP_SWITCH(op)   goto *g_op_labels[(size_t)op];
    #define OP_CASE(op)     op_ ## op
    #define OP_DEFAULT      op_default
    #define OP_BREAK        goto op_loop;
    #define OP_RETURN       op_return:
#else
    #define OP_LOOP         for (;;)
    #define OP_SWITCH(op)   switch (op)
    #define OP_CASE(op)     case Opcode::op
    #define OP_DEFAULT      default
    #define OP_BREAK        break;
    #define OP_RETURN
#endif


void Machine::call(const Function& function, const Machine::InvokeCallback& cb)
{
    m_stack.push_frame(function);
    try {
        run(cb);
        assert(m_stack.size() == function.effective_return_type().size());
    } catch (RuntimeError& e) {
        // unwind the whole stack, fill StackTrace in the ScriptError
        e.set_stack_trace(m_stack.make_trace());
        throw;
    }
}


void Machine::run(const InvokeCallback& cb)
{
#ifdef XCI_SCRIPT_COMPUTED_GOTO
    static void* g_op_labels[] = {
        [(size_t) Opcode::Noop] = &&op_Noop,
        [(size_t) Opcode::LogicalNot] = &&op_LogicalNot,
        [(size_t) Opcode::LogicalOr] = &&op_LogicalOr,
        [(size_t) Opcode::LogicalAnd] = &&op_LogicalAnd,

        [(size_t) Opcode::BitwiseNot_8] = &&op_BitwiseNot_8,
        [(size_t) Opcode::BitwiseNot_16] = &&op_BitwiseNot_16,
        [(size_t) Opcode::BitwiseNot_32] = &&op_BitwiseNot_32,
        [(size_t) Opcode::BitwiseNot_64] = &&op_BitwiseNot_64,
        [(size_t) Opcode::BitwiseNot_128] = &&op_BitwiseNot_128,
        [(size_t) Opcode::BitwiseOr_8] = &&op_BitwiseOr_8,
        [(size_t) Opcode::BitwiseOr_16] = &&op_BitwiseOr_16,
        [(size_t) Opcode::BitwiseOr_32] = &&op_BitwiseOr_32,
        [(size_t) Opcode::BitwiseOr_64] = &&op_BitwiseOr_64,
        [(size_t) Opcode::BitwiseOr_128] = &&op_BitwiseOr_128,
        [(size_t) Opcode::BitwiseAnd_8] = &&op_BitwiseAnd_8,
        [(size_t) Opcode::BitwiseAnd_16] = &&op_BitwiseAnd_16,
        [(size_t) Opcode::BitwiseAnd_32] = &&op_BitwiseAnd_32,
        [(size_t) Opcode::BitwiseAnd_64] = &&op_BitwiseAnd_64,
        [(size_t) Opcode::BitwiseAnd_128] = &&op_BitwiseAnd_128,
        [(size_t) Opcode::BitwiseXor_8] = &&op_BitwiseXor_8,
        [(size_t) Opcode::BitwiseXor_16] = &&op_BitwiseXor_16,
        [(size_t) Opcode::BitwiseXor_32] = &&op_BitwiseXor_32,
        [(size_t) Opcode::BitwiseXor_64] = &&op_BitwiseXor_64,
        [(size_t) Opcode::BitwiseXor_128] = &&op_BitwiseXor_128,

        [(size_t) Opcode::ShiftLeft_8] = &&op_ShiftLeft_8,
        [(size_t) Opcode::ShiftLeft_16] = &&op_ShiftLeft_16,
        [(size_t) Opcode::ShiftLeft_32] = &&op_ShiftLeft_32,
        [(size_t) Opcode::ShiftLeft_64] = &&op_ShiftLeft_64,
        [(size_t) Opcode::ShiftLeft_128] = &&op_ShiftLeft_128,
        [(size_t) Opcode::ShiftRight_8] = &&op_ShiftRight_8,
        [(size_t) Opcode::ShiftRight_16] = &&op_ShiftRight_16,
        [(size_t) Opcode::ShiftRight_32] = &&op_ShiftRight_32,
        [(size_t) Opcode::ShiftRight_64] = &&op_ShiftRight_64,
        [(size_t) Opcode::ShiftRight_128] = &&op_ShiftRight_128,
        [(size_t) Opcode::ShiftRightSE_8] = &&op_ShiftRightSE_8,
        [(size_t) Opcode::ShiftRightSE_16] = &&op_ShiftRightSE_16,
        [(size_t) Opcode::ShiftRightSE_32] = &&op_ShiftRightSE_32,
        [(size_t) Opcode::ShiftRightSE_64] = &&op_ShiftRightSE_64,
        [(size_t) Opcode::ShiftRightSE_128] = &&op_ShiftRightSE_128,

        [(size_t) Opcode::Execute] = &&op_Execute,
        [(size_t) Opcode::Ret] = &&op_Ret,

        [(size_t) Opcode::Cast] = &&op_Cast,
        [(size_t) Opcode::Equal] = &&op_Equal,
        [(size_t) Opcode::NotEqual] = &&op_NotEqual,
        [(size_t) Opcode::LessEqual] = &&op_LessEqual,
        [(size_t) Opcode::GreaterEqual] = &&op_GreaterEqual,
        [(size_t) Opcode::LessThan] = &&op_LessThan,
        [(size_t) Opcode::GreaterThan] = &&op_GreaterThan,

        [(size_t) Opcode::Neg] = &&op_Neg,
        [(size_t) Opcode::Add] = &&op_Add,
        [(size_t) Opcode::Sub] = &&op_Sub,
        [(size_t) Opcode::Mul] = &&op_Mul,
        [(size_t) Opcode::Div] = &&op_Div,
        [(size_t) Opcode::Mod] = &&op_Mod,
        [(size_t) Opcode::Exp] = &&op_Exp,

        [(size_t) Opcode::UnsafeAdd] = &&op_UnsafeAdd,
        [(size_t) Opcode::UnsafeSub] = &&op_UnsafeSub,
        [(size_t) Opcode::UnsafeMul] = &&op_UnsafeMul,
        [(size_t) Opcode::UnsafeDiv] = &&op_UnsafeDiv,
        [(size_t) Opcode::UnsafeMod] = &&op_UnsafeMod,

        [(size_t) Opcode::Jump] = &&op_Jump,
        [(size_t) Opcode::JumpIfNot] = &&op_JumpIfNot,

        [(size_t) Opcode::LoadStatic] = &&op_LoadStatic,
        [(size_t) Opcode::LoadModule] = &&op_LoadModule,
        [(size_t) Opcode::LoadFunction] = &&op_LoadFunction,

        [(size_t) Opcode::Call0] = &&op_Call0,
        [(size_t) Opcode::TailCall0] = &&op_TailCall0,
        [(size_t) Opcode::Call1] = &&op_Call1,
        [(size_t) Opcode::TailCall1] = &&op_TailCall1,

        [(size_t) Opcode::MakeClosure] = &&op_MakeClosure,
        [(size_t) Opcode::SetBase] = &&op_SetBase,
        [(size_t) Opcode::IncRef] = &&op_IncRef,
        [(size_t) Opcode::DecRef] = &&op_DecRef,

        [(size_t) Opcode::ListSubscript] = &&op_ListSubscript,
        [(size_t) Opcode::ListLength] = &&op_ListLength,
        [(size_t) Opcode::ListSlice] = &&op_ListSlice,
        [(size_t) Opcode::ListConcat] = &&op_ListConcat,

        [(size_t) Opcode::Invoke] = &&op_Invoke,

        [(size_t) Opcode::Call] = &&op_Call,
        [(size_t) Opcode::TailCall] = &&op_TailCall,
        [(size_t) Opcode::MakeList] = &&op_MakeList,
        [(size_t) Opcode::Copy] = &&op_Copy,
        [(size_t) Opcode::Drop] = &&op_Drop,
        [(size_t) Opcode::Swap] = &&op_Swap,

        [(size_t) Opcode::Annotation] = &&op_default,
    };
#endif

    // Avoid double-recursion - move these pointers instead (we already have a stack)
    const Function* function = &m_stack.frame().function;
    assert(function->is_bytecode());
    auto it = function->bytecode().begin() + (std::ptrdiff_t) m_stack.frame().instruction;
    auto base = m_stack.frame().base;

    auto call_fun = [this, &function, &it, &base](const Function& fn) {
        if (fn.is_native()) {
            fn.call_native(m_stack);
            return;
        }
        // return address
        m_stack.frame().instruction = it - function->bytecode().begin();
        assert(fn.is_bytecode());
        m_stack.push_frame(fn);
        it = fn.bytecode().begin();
        base = m_stack.frame().base;
        function = &fn;
        if (m_call_enter_cb)
            m_call_enter_cb(fn);
    };

    auto tail_call_fun = [this, &function, &it, &base](const Function& fn) {
        assert(fn.is_bytecode());
        if (m_call_exit_cb)
            m_call_exit_cb(*function);
        m_stack.pop_frame();
        m_stack.push_frame(fn);
        it = fn.bytecode().begin();
        base = m_stack.frame().base;
        function = &fn;
        if (m_call_enter_cb)
            m_call_enter_cb(fn);
    };

    // read type argument as generated by intrinsic: `__type_index<T>`
    auto read_type_arg = [&it, &function]() -> const TypeInfo& {
        // LEB128 encoding of a type_index
        auto index = leb128_decode<Index>(it);
        return get_type_info_unchecked(function->module().module_manager(), index);
    };

    // Run function code
    Opcode opcode;
    if (m_call_enter_cb)
        m_call_enter_cb(*function);
    OP_LOOP {
        if (it == function->bytecode().end()) {
            [[unlikely]]
            throw bad_instruction("reached end of code (missing RET)");
        }

        if (m_bytecode_trace_cb)
            m_bytecode_trace_cb(*function, it);

        opcode = static_cast<Opcode>(*it++);
        OP_SWITCH (opcode) {
            OP_CASE(Noop):
                OP_BREAK

            OP_CASE(Ret):
                // return from function
                if (m_call_exit_cb)
                    m_call_exit_cb(*function);

                // no more stack frames?
                if (m_stack.n_frames() == 1) {
                    assert(function == &m_stack.frame().function);
                    m_stack.pop_frame();
                    return;
                }

                // return into previous call location
                m_stack.pop_frame();
                function = &m_stack.frame().function;
                it = function->bytecode().begin() + (std::ptrdiff_t) m_stack.frame().instruction;
                base = m_stack.frame().base;
                OP_BREAK

            OP_CASE(LogicalOr):
            OP_CASE(LogicalAnd): {
                auto lhs = m_stack.pull<value::Bool>();
                auto rhs = m_stack.pull<value::Bool>();
                switch (opcode) {
                    case Opcode::LogicalOr:   m_stack.push(lhs.binary_op<std::logical_or<>, true>(rhs)); break;
                    case Opcode::LogicalAnd:  m_stack.push(lhs.binary_op<std::logical_and<>, true>(rhs)); break;
                    default: break;
                }
            } OP_BREAK

            OP_CASE(Equal):
            OP_CASE(NotEqual):
            OP_CASE(LessEqual):
            OP_CASE(GreaterEqual):
            OP_CASE(LessThan):
            OP_CASE(GreaterThan): {
                const auto arg = *it++;
                const auto lhs_type = decode_arg_type(arg >> 4);
                const auto rhs_type = decode_arg_type(arg & 0xf);
                if (lhs_type == Type::Unknown || rhs_type == Type::Unknown || lhs_type != rhs_type)
                    throw not_implemented(format("opcode: {} lhs type: {:x} rhs type: {:x}",
                            opcode, arg >> 4, arg & 0xf));
                auto lhs = m_stack.pull(TypeInfo{lhs_type});
                auto rhs = m_stack.pull(TypeInfo{rhs_type});
                switch (opcode) {
                    case Opcode::Equal: m_stack.push(lhs.binary_op<std::equal_to<>>(rhs)); break;
                    case Opcode::NotEqual: m_stack.push(lhs.binary_op<std::not_equal_to<>>(rhs)); break;
                    case Opcode::LessEqual: m_stack.push(lhs.binary_op<std::less_equal<>>(rhs)); break;
                    case Opcode::GreaterEqual: m_stack.push(lhs.binary_op<std::greater_equal<>>(rhs)); break;
                    case Opcode::LessThan: m_stack.push(lhs.binary_op<std::less<>>(rhs)); break;
                    case Opcode::GreaterThan: m_stack.push(lhs.binary_op<std::greater<>>(rhs)); break;
                    default: break;
                }
            } OP_BREAK

            OP_CASE(BitwiseOr_8): {
                auto lhs = m_stack.pull<value::UInt8>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(lhs.binary_op<std::bit_or<>, true>(rhs));
            } OP_BREAK
            OP_CASE(BitwiseAnd_8): {
                auto lhs = m_stack.pull<value::UInt8>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(lhs.binary_op<std::bit_and<>, true>(rhs));
            } OP_BREAK
            OP_CASE(BitwiseXor_8): {
                auto lhs = m_stack.pull<value::UInt8>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(lhs.binary_op<std::bit_xor<>, true>(rhs));
            } OP_BREAK
            OP_CASE(BitwiseOr_16): {
                auto lhs = m_stack.pull<value::UInt16>();
                auto rhs = m_stack.pull<value::UInt16>();
                m_stack.push(lhs.binary_op<std::bit_or<>, true>(rhs));
            } OP_BREAK
            OP_CASE(BitwiseAnd_16): {
                auto lhs = m_stack.pull<value::UInt16>();
                auto rhs = m_stack.pull<value::UInt16>();
                m_stack.push(lhs.binary_op<std::bit_and<>, true>(rhs));
            } OP_BREAK
            OP_CASE(BitwiseXor_16): {
                auto lhs = m_stack.pull<value::UInt16>();
                auto rhs = m_stack.pull<value::UInt16>();
                m_stack.push(lhs.binary_op<std::bit_xor<>, true>(rhs));
            } OP_BREAK
            OP_CASE(BitwiseOr_32): {
                auto lhs = m_stack.pull<value::UInt32>();
                auto rhs = m_stack.pull<value::UInt32>();
                m_stack.push(lhs.binary_op<std::bit_or<>, true>(rhs));
            } OP_BREAK
            OP_CASE(BitwiseAnd_32): {
                auto lhs = m_stack.pull<value::UInt32>();
                auto rhs = m_stack.pull<value::UInt32>();
                m_stack.push(lhs.binary_op<std::bit_and<>, true>(rhs));
            } OP_BREAK
            OP_CASE(BitwiseXor_32): {
                auto lhs = m_stack.pull<value::UInt32>();
                auto rhs = m_stack.pull<value::UInt32>();
                m_stack.push(lhs.binary_op<std::bit_xor<>, true>(rhs));
            } OP_BREAK
            OP_CASE(BitwiseOr_64): {
                auto lhs = m_stack.pull<value::UInt64>();
                auto rhs = m_stack.pull<value::UInt64>();
                m_stack.push(lhs.binary_op<std::bit_or<>, true>(rhs));
            } OP_BREAK
            OP_CASE(BitwiseAnd_64): {
                auto lhs = m_stack.pull<value::UInt64>();
                auto rhs = m_stack.pull<value::UInt64>();
                m_stack.push(lhs.binary_op<std::bit_and<>, true>(rhs));
            } OP_BREAK
            OP_CASE(BitwiseXor_64): {
                auto lhs = m_stack.pull<value::UInt64>();
                auto rhs = m_stack.pull<value::UInt64>();
                m_stack.push(lhs.binary_op<std::bit_xor<>, true>(rhs));
            } OP_BREAK
            OP_CASE(BitwiseOr_128): {
                auto lhs = m_stack.pull<value::UInt128>();
                auto rhs = m_stack.pull<value::UInt128>();
                m_stack.push(lhs.binary_op<std::bit_or<>, true>(rhs));
            } OP_BREAK
            OP_CASE(BitwiseAnd_128): {
                auto lhs = m_stack.pull<value::UInt128>();
                auto rhs = m_stack.pull<value::UInt128>();
                m_stack.push(lhs.binary_op<std::bit_and<>, true>(rhs));
            } OP_BREAK
            OP_CASE(BitwiseXor_128): {
                auto lhs = m_stack.pull<value::UInt128>();
                auto rhs = m_stack.pull<value::UInt128>();
                m_stack.push(lhs.binary_op<std::bit_xor<>, true>(rhs));
            } OP_BREAK

            OP_CASE(ShiftLeft_8): {
                auto lhs = m_stack.pull<value::UInt8>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_left(lhs.value(), rhs.value())));
            } OP_BREAK
            OP_CASE(ShiftRight_8): {
                auto lhs = m_stack.pull<value::UInt8>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_right(lhs.value(), rhs.value())));
            } OP_BREAK
            OP_CASE(ShiftRightSE_8): {
                auto lhs = m_stack.pull<value::Int8>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_right(lhs.value(), rhs.value())));
            } OP_BREAK
            OP_CASE(ShiftLeft_16): {
                auto lhs = m_stack.pull<value::UInt16>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_left(lhs.value(), rhs.value())));
            } OP_BREAK
            OP_CASE(ShiftRight_16): {
                auto lhs = m_stack.pull<value::UInt16>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_right(lhs.value(), rhs.value())));
            } OP_BREAK
            OP_CASE(ShiftRightSE_16): {
                auto lhs = m_stack.pull<value::Int16>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_right(lhs.value(), rhs.value())));
            } OP_BREAK
            OP_CASE(ShiftLeft_32): {
                auto lhs = m_stack.pull<value::UInt32>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_left(lhs.value(), rhs.value())));
            } OP_BREAK
            OP_CASE(ShiftRight_32): {
                auto lhs = m_stack.pull<value::UInt32>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_right(lhs.value(), rhs.value())));
            } OP_BREAK
            OP_CASE(ShiftRightSE_32): {
                auto lhs = m_stack.pull<value::Int32>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_right(lhs.value(), rhs.value())));
            } OP_BREAK
            OP_CASE(ShiftLeft_64): {
                auto lhs = m_stack.pull<value::UInt64>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_left(lhs.value(), rhs.value())));
            } OP_BREAK
            OP_CASE(ShiftRight_64): {
                auto lhs = m_stack.pull<value::UInt64>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_right(lhs.value(), rhs.value())));
            } OP_BREAK
            OP_CASE(ShiftRightSE_64): {
                auto lhs = m_stack.pull<value::Int64>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_right(lhs.value(), rhs.value())));
            } OP_BREAK
            OP_CASE(ShiftLeft_128): {
                auto lhs = m_stack.pull<value::UInt128>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_left(lhs.value(), rhs.value())));
            } OP_BREAK
            OP_CASE(ShiftRight_128): {
                auto lhs = m_stack.pull<value::UInt128>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_right(lhs.value(), rhs.value())));
            } OP_BREAK
            OP_CASE(ShiftRightSE_128): {
                auto lhs = m_stack.pull<value::Int128>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(Value(builtin::shift_right(lhs.value(), rhs.value())));
            } OP_BREAK

            OP_CASE(Add):
            OP_CASE(Sub):
            OP_CASE(Mul):
            OP_CASE(Div):
            OP_CASE(Mod):
            OP_CASE(Exp):
            OP_CASE(UnsafeAdd):
            OP_CASE(UnsafeSub):
            OP_CASE(UnsafeMul):
            OP_CASE(UnsafeDiv):
            OP_CASE(UnsafeMod): {
                const auto arg = *it++;
                const auto lhs_type = decode_arg_type(arg >> 4);
                const auto rhs_type = decode_arg_type(arg & 0xf);
                if (lhs_type == Type::Unknown || rhs_type == Type::Unknown || lhs_type != rhs_type)
                    throw not_implemented(format("opcode: {} lhs type: {:x} rhs type: {:x}",
                            opcode, arg >> 4, arg & 0xf));
                auto lhs = m_stack.pull(TypeInfo{lhs_type});
                auto rhs = m_stack.pull(TypeInfo{rhs_type});
                switch (opcode) {
                    case Opcode::Add: m_stack.push(lhs.binary_op<builtin::Add>(rhs)); break;
                    case Opcode::Sub: m_stack.push(lhs.binary_op<builtin::Sub>(rhs)); break;
                    case Opcode::Mul: m_stack.push(lhs.binary_op<builtin::Mul>(rhs)); break;
                    case Opcode::Div: m_stack.push(lhs.binary_op<builtin::Div>(rhs)); break;
                    case Opcode::Mod: m_stack.push(lhs.binary_op<builtin::Mod>(rhs)); break;
                    case Opcode::Exp: m_stack.push(lhs.binary_op<builtin::Exp>(rhs)); break;
                    case Opcode::UnsafeAdd: m_stack.push(lhs.binary_op<builtin::UnsafeAdd>(rhs)); break;
                    case Opcode::UnsafeSub: m_stack.push(lhs.binary_op<builtin::UnsafeSub>(rhs)); break;
                    case Opcode::UnsafeMul: m_stack.push(lhs.binary_op<builtin::UnsafeMul>(rhs)); break;
                    case Opcode::UnsafeDiv: m_stack.push(lhs.binary_op<builtin::UnsafeDiv>(rhs)); break;
                    case Opcode::UnsafeMod: m_stack.push(lhs.binary_op<builtin::UnsafeMod>(rhs)); break;
                    default: XCI_UNREACHABLE;
                }
            } OP_BREAK

            OP_CASE(LogicalNot):
                m_stack.push(Value(! m_stack.pull<value::Bool>().value()));
                OP_BREAK

            OP_CASE(BitwiseNot_8):
                m_stack.push(Value(~ m_stack.pull<value::UInt8>().value()));
                OP_BREAK
            OP_CASE(BitwiseNot_16):
                m_stack.push(Value(~ m_stack.pull<value::UInt16>().value()));
                OP_BREAK
            OP_CASE(BitwiseNot_32):
                m_stack.push(Value(~ m_stack.pull<value::UInt32>().value()));
                OP_BREAK
            OP_CASE(BitwiseNot_64):
                m_stack.push(Value(~ m_stack.pull<value::UInt64>().value()));
                OP_BREAK
            OP_CASE(BitwiseNot_128):
                m_stack.push(Value(~ m_stack.pull<value::UInt128>().value()));
                OP_BREAK

            OP_CASE(Neg): {
                const auto arg = *it++;
                const auto type = decode_arg_type(arg & 0xf);
                if (type == Type::Unknown)
                    throw not_implemented(format("opcode: {} type: {:x}",
                            opcode, arg & 0xf));
                auto v = m_stack.pull(TypeInfo{type});
                v.negate();
                m_stack.push(v);
            } OP_BREAK

            OP_CASE(ListSubscript): {
                const auto& elem_ti = read_type_arg();
                auto lhs = m_stack.pull_typed(ti_list(TypeInfo(elem_ti)));
                auto rhs = m_stack.pull<value::Int>();
                auto idx = rhs.value();
                auto len = lhs.get<ListV>().length();
                if (idx < 0)
                    idx += (int) len;
                if (idx < 0 || (size_t) idx >= len) {
                    lhs.decref();
                    throw index_out_of_bounds(idx, len);
                }
                const Value item = lhs.get<ListV>().value_at(idx, elem_ti);
                item.incref();
                lhs.decref();
                m_stack.push(item);
            } OP_BREAK

            OP_CASE(ListLength): {
                const auto& elem_ti = read_type_arg();
                auto arg = m_stack.pull_typed(ti_list(TypeInfo(elem_ti)));
                auto len = arg.get<ListV>().length();
                arg.decref();
                m_stack.push(value::UInt(len));
            } OP_BREAK

            OP_CASE(ListSlice): {
                const auto& elem_ti = read_type_arg();
                auto list = m_stack.pull_typed(ti_list(TypeInfo(elem_ti)));
                auto idx1 = m_stack.pull<value::Int>().value();
                auto idx2 = m_stack.pull<value::Int>().value();
                auto step = m_stack.pull<value::Int>().value();
                list.get<ListV>().slice(idx1, idx2, step, elem_ti);
                m_stack.push(list);
            } OP_BREAK

            OP_CASE(ListConcat): {
                const auto& elem_ti = read_type_arg();
                auto lhs = m_stack.pull_typed(ti_list(TypeInfo(elem_ti)));
                auto rhs = m_stack.pull_typed(ti_list(TypeInfo(elem_ti)));
                lhs.get<ListV>().extend(rhs.get<ListV>(), elem_ti);
                rhs.decref();
                m_stack.push(lhs);
            } OP_BREAK

            OP_CASE(Cast): {
                // TODO: possible optimization when truncating integers
                //       or extending unsigned integers: do not pull the value,
                //       but truncate or extend it directly in the stack
                const auto arg = *it++;
                const auto from_type = decode_arg_type(arg >> 4);
                const auto to_type = decode_arg_type(arg & 0xf);
                if (from_type == Type::Unknown)
                    throw not_implemented(format("cast from: {:x}", arg >> 4));
                if (to_type == Type::Unknown)
                    throw not_implemented(format("cast to: {:x}", arg & 0xf));
                auto from = m_stack.pull(TypeInfo{from_type});
                auto to = create_value(TypeInfo{to_type});
                if (!to.cast_from(from))
                    throw not_implemented(format("cast {} to {}",
                                         TypeInfo{from_type}, TypeInfo{to_type}));
                m_stack.push(to);
            } OP_BREAK

            OP_CASE(Invoke): {
                const auto& type_info = read_type_arg();
                cb(m_stack.pull_typed(type_info));
            } OP_BREAK

            OP_CASE(Execute): {
                auto o = m_stack.pull<value::Closure>();
                auto closure = o.closure();
                for (size_t i = closure.length(); i != 0;) {
                    m_stack.push(closure.value_at(--i));
                }
                call_fun(*o.function());
                o.decref();
            } OP_BREAK

            OP_CASE(LoadStatic): {
                auto arg = leb128_decode<Index>(it);
                const auto& o = function->module().get_value(arg);
                m_stack.push(o);
                o.incref();
            } OP_BREAK

            OP_CASE(LoadFunction): {
                auto arg = leb128_decode<Index>(it);
                auto& fn = function->module().get_function(arg);
                m_stack.push(value::Closure(fn));
            } OP_BREAK

            OP_CASE(LoadModule): {
                auto arg = leb128_decode<Index>(it);
                auto& mod = (arg == no_index)? function->module() : function->module().get_imported_module(arg);
                m_stack.push(value::Module(mod));
            } OP_BREAK

            OP_CASE(SetBase): {
                const auto level = leb128_decode<size_t>(it);
                base = m_stack.frame(m_stack.n_frames() - 1 - level).base;
            } OP_BREAK

            OP_CASE(Copy): {
                const auto arg1 = leb128_decode<size_t>(it);
                const auto addr = arg1 + m_stack.to_rel(base); // arg1 + base
                const auto size = leb128_decode<size_t>(it); // arg2
                m_stack.copy(addr, size);
            } OP_BREAK

            OP_CASE(Drop): {
                const auto addr = leb128_decode<size_t>(it);
                const auto size = leb128_decode<size_t>(it);
                m_stack.drop(addr, size);
            } OP_BREAK

            OP_CASE(Swap): {
                const auto arg1 = leb128_decode<size_t>(it);
                const auto arg2 = leb128_decode<size_t>(it);
                m_stack.swap(arg1, arg2);
            } OP_BREAK

            OP_CASE(Call0):
            OP_CASE(Call1):
            OP_CASE(Call):
            OP_CASE(TailCall0):
            OP_CASE(TailCall1):
            OP_CASE(TailCall): {
                // get the function's module
                Module* module;
                if (opcode == Opcode::Call0 || opcode == Opcode::TailCall0) {
                    module = &function->module();
                } else {
                    Index idx;
                    if (opcode == Opcode::Call1 || opcode == Opcode::TailCall1) {
                        idx = 0;
                    } else {
                        // read arg1
                        idx = leb128_decode<Index>(it);
                    }
                    module = &function->module().get_imported_module(idx);
                }
                // call function from the module
                auto arg = leb128_decode<Index>(it);
                auto& fn = module->get_function(arg);
                if (opcode == Opcode::TailCall0 || opcode == Opcode::TailCall1 || opcode == Opcode::TailCall)
                    tail_call_fun(fn);
                else
                    call_fun(fn);
            } OP_BREAK

            OP_CASE(MakeList): {
                const auto num_elems = leb128_decode<uint32_t>(it);
                const auto& elem_ti = read_type_arg();
                // move list contents from stack to heap
                ListV list(num_elems, elem_ti, m_stack.data());
                m_stack.drop(0, num_elems * elem_ti.size());
                // push list handle back to stack
                m_stack.push(Value{std::move(list)});
            } OP_BREAK

            OP_CASE(MakeClosure): {
                auto arg = leb128_decode<Index>(it);
                // get function
                auto& fn = function->module().get_function(arg);
                // pull nonlocals
                Values closure;
                closure.reserve(fn.nonlocals().size());
                for (const auto & ti : fn.nonlocals()) {
                    closure.add(m_stack.pull(ti));
                }
                // push closure
                m_stack.push(value::Closure{fn, std::move(closure)});
            } OP_BREAK

            OP_CASE(IncRef): {
                auto arg = leb128_decode<Stack::StackRel>(it);
                const HeapSlot slot {static_cast<byte*>(m_stack.get_ptr(arg))};
                slot.incref();
            } OP_BREAK

            OP_CASE(DecRef): {
                auto arg = leb128_decode<Stack::StackRel>(it);
                const HeapSlot slot {static_cast<byte*>(m_stack.get_ptr(arg))};
                if (slot.decref())
                    m_stack.clear_ptr(arg);  // without this, stack dump would read after use
            } OP_BREAK

            OP_CASE(Jump): {
                auto arg = *it++;
                it += arg;
            } OP_BREAK

            OP_CASE(JumpIfNot): {
                auto arg = *it++;
                auto cond = m_stack.pull<value::Bool>();
                if (!cond.value()) {
                    it += arg;
                }
            } OP_BREAK

            OP_DEFAULT:
                throw not_implemented(format("opcode {}", opcode));
        }
    }
}


} // namespace xci::script
