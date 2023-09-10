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
    if (m_call_enter_cb)
        m_call_enter_cb(*function);
    for (;;) {
        if (it == function->bytecode().end())
            throw bad_instruction("reached end of code (missing RET)");

        if (m_bytecode_trace_cb)
            m_bytecode_trace_cb(*function, it);

        auto opcode = static_cast<Opcode>(*it++);
        switch (opcode) {
            case Opcode::Noop:
                break;

            case Opcode::Ret:
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
                break;

            case Opcode::LogicalOr:
            case Opcode::LogicalAnd: {
                auto lhs = m_stack.pull<value::Bool>();
                auto rhs = m_stack.pull<value::Bool>();
                switch (opcode) {
                    case Opcode::LogicalOr:   m_stack.push(lhs.binary_op<std::logical_or<>, true>(rhs)); break;
                    case Opcode::LogicalAnd:  m_stack.push(lhs.binary_op<std::logical_and<>, true>(rhs)); break;
                    default: break;
                }
                break;
            }

            case Opcode::Equal:
            case Opcode::NotEqual:
            case Opcode::LessEqual:
            case Opcode::GreaterEqual:
            case Opcode::LessThan:
            case Opcode::GreaterThan: {
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
                break;
            }

            case Opcode::BitwiseOr_8: {
                auto lhs = m_stack.pull<value::UInt8>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(lhs.binary_op<std::bit_or<>, true>(rhs));
                break;
            }
            case Opcode::BitwiseAnd_8: {
                auto lhs = m_stack.pull<value::UInt8>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(lhs.binary_op<std::bit_and<>, true>(rhs));
                break;
            }
            case Opcode::BitwiseXor_8: {
                auto lhs = m_stack.pull<value::UInt8>();
                auto rhs = m_stack.pull<value::UInt8>();
                m_stack.push(lhs.binary_op<std::bit_xor<>, true>(rhs));
                break;
            }
            case Opcode::BitwiseOr_16: {
                auto lhs = m_stack.pull<value::UInt16>();
                auto rhs = m_stack.pull<value::UInt16>();
                m_stack.push(lhs.binary_op<std::bit_or<>, true>(rhs));
                break;
            }
            case Opcode::BitwiseAnd_16: {
                auto lhs = m_stack.pull<value::UInt16>();
                auto rhs = m_stack.pull<value::UInt16>();
                m_stack.push(lhs.binary_op<std::bit_and<>, true>(rhs));
                break;
            }
            case Opcode::BitwiseXor_16: {
                auto lhs = m_stack.pull<value::UInt16>();
                auto rhs = m_stack.pull<value::UInt16>();
                m_stack.push(lhs.binary_op<std::bit_xor<>, true>(rhs));
                break;
            }
            case Opcode::BitwiseOr_32: {
                auto lhs = m_stack.pull<value::UInt32>();
                auto rhs = m_stack.pull<value::UInt32>();
                m_stack.push(lhs.binary_op<std::bit_or<>, true>(rhs));
                break;
            }
            case Opcode::BitwiseAnd_32: {
                auto lhs = m_stack.pull<value::UInt32>();
                auto rhs = m_stack.pull<value::UInt32>();
                m_stack.push(lhs.binary_op<std::bit_and<>, true>(rhs));
                break;
            }
            case Opcode::BitwiseXor_32: {
                auto lhs = m_stack.pull<value::UInt32>();
                auto rhs = m_stack.pull<value::UInt32>();
                m_stack.push(lhs.binary_op<std::bit_xor<>, true>(rhs));
                break;
            }
            case Opcode::BitwiseOr_64: {
                auto lhs = m_stack.pull<value::UInt64>();
                auto rhs = m_stack.pull<value::UInt64>();
                m_stack.push(lhs.binary_op<std::bit_or<>, true>(rhs));
                break;
            }
            case Opcode::BitwiseAnd_64: {
                auto lhs = m_stack.pull<value::UInt64>();
                auto rhs = m_stack.pull<value::UInt64>();
                m_stack.push(lhs.binary_op<std::bit_and<>, true>(rhs));
                break;
            }
            case Opcode::BitwiseXor_64: {
                auto lhs = m_stack.pull<value::UInt64>();
                auto rhs = m_stack.pull<value::UInt64>();
                m_stack.push(lhs.binary_op<std::bit_xor<>, true>(rhs));
                break;
            }
            case Opcode::BitwiseOr_128: {
                auto lhs = m_stack.pull<value::UInt128>();
                auto rhs = m_stack.pull<value::UInt128>();
                m_stack.push(lhs.binary_op<std::bit_or<>, true>(rhs));
                break;
            }
            case Opcode::BitwiseAnd_128: {
                auto lhs = m_stack.pull<value::UInt128>();
                auto rhs = m_stack.pull<value::UInt128>();
                m_stack.push(lhs.binary_op<std::bit_and<>, true>(rhs));
                break;
            }
            case Opcode::BitwiseXor_128: {
                auto lhs = m_stack.pull<value::UInt128>();
                auto rhs = m_stack.pull<value::UInt128>();
                m_stack.push(lhs.binary_op<std::bit_xor<>, true>(rhs));
                break;
            }

            case Opcode::ShiftLeft:
            case Opcode::ShiftRight: {
                const auto arg = *it++;
                const auto lhs_type = decode_arg_type(arg >> 4);
                const auto rhs_type = decode_arg_type(arg & 0xf);
                if (lhs_type == Type::Unknown || rhs_type == Type::Unknown || lhs_type != rhs_type)
                    throw not_implemented(format("opcode: {} lhs type: {:x} rhs type: {:x}",
                            opcode, arg >> 4, arg & 0xf));
                auto lhs = m_stack.pull(TypeInfo{lhs_type});
                auto rhs = m_stack.pull(TypeInfo{rhs_type});
                switch (opcode) {
                    case Opcode::ShiftLeft: m_stack.push(lhs.binary_op<builtin::shift_left, true>(rhs)); break;
                    case Opcode::ShiftRight: m_stack.push(lhs.binary_op<builtin::shift_right, true>(rhs)); break;
                    default: XCI_UNREACHABLE;
                }
                break;
            }

            case Opcode::Add:
            case Opcode::Sub:
            case Opcode::Mul:
            case Opcode::Div:
            case Opcode::Exp: {
                const auto arg = *it++;
                const auto lhs_type = decode_arg_type(arg >> 4);
                const auto rhs_type = decode_arg_type(arg & 0xf);
                if (lhs_type == Type::Unknown || rhs_type == Type::Unknown || lhs_type != rhs_type)
                    throw not_implemented(format("opcode: {} lhs type: {:x} rhs type: {:x}",
                            opcode, arg >> 4, arg & 0xf));
                auto lhs = m_stack.pull(TypeInfo{lhs_type});
                auto rhs = m_stack.pull(TypeInfo{rhs_type});
                switch (opcode) {
                    case Opcode::Add: m_stack.push(lhs.binary_op<std::plus<>>(rhs)); break;
                    case Opcode::Sub: m_stack.push(lhs.binary_op<std::minus<>>(rhs)); break;
                    case Opcode::Mul: m_stack.push(lhs.binary_op<std::multiplies<>>(rhs)); break;
                    case Opcode::Div: m_stack.push(lhs.binary_op<std::divides<>>(rhs)); break;
                    case Opcode::Exp: m_stack.push(lhs.binary_op<builtin::exp>(rhs)); break;
                    default: XCI_UNREACHABLE;
                }
                break;
            }

            case Opcode::Mod: {
                const auto arg = *it++;
                const auto lhs_type = decode_arg_type(arg >> 4);
                const auto rhs_type = decode_arg_type(arg & 0xf);
                if (lhs_type == Type::Unknown || rhs_type == Type::Unknown || lhs_type != rhs_type)
                    throw not_implemented(format("modulus lhs: {:x} rhs: {:x}",
                            arg >> 4, arg & 0xf));
                auto lhs = m_stack.pull(TypeInfo{lhs_type});
                auto rhs = m_stack.pull(TypeInfo{rhs_type});
                if (!lhs.modulus(rhs))
                    throw not_implemented(format("modulus lhs: {:x} rhs: {:x}",
                            arg >> 4, arg & 0xf));
                m_stack.push(lhs);
                break;
            }

            case Opcode::LogicalNot:
                m_stack.push(Value(! m_stack.pull<value::Bool>().value()));
                break;

            case Opcode::BitwiseNot_8:
                m_stack.push(Value(~ m_stack.pull<value::UInt8>().value()));
                break;
            case Opcode::BitwiseNot_16:
                m_stack.push(Value(~ m_stack.pull<value::UInt16>().value()));
                break;
            case Opcode::BitwiseNot_32:
                m_stack.push(Value(~ m_stack.pull<value::UInt32>().value()));
                break;
            case Opcode::BitwiseNot_64:
                m_stack.push(Value(~ m_stack.pull<value::UInt64>().value()));
                break;
            case Opcode::BitwiseNot_128:
                m_stack.push(Value(~ m_stack.pull<value::UInt128>().value()));
                break;

            case Opcode::Neg: {
                const auto arg = *it++;
                const auto type = decode_arg_type(arg & 0xf);
                if (type == Type::Unknown)
                    throw not_implemented(format("opcode: {} type: {:x}",
                            opcode, arg & 0xf));
                auto v = m_stack.pull(TypeInfo{type});
                v.negate();
                m_stack.push(v);
                break;
            }

            case Opcode::ListSubscript: {
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
                break;
            }

            case Opcode::ListLength: {
                const auto& elem_ti = read_type_arg();
                auto arg = m_stack.pull_typed(ti_list(TypeInfo(elem_ti)));
                auto len = arg.get<ListV>().length();
                arg.decref();
                m_stack.push(value::UInt(len));
                break;
            }

            case Opcode::ListSlice: {
                const auto& elem_ti = read_type_arg();
                auto list = m_stack.pull_typed(ti_list(TypeInfo(elem_ti)));
                auto idx1 = m_stack.pull<value::Int>().value();
                auto idx2 = m_stack.pull<value::Int>().value();
                auto step = m_stack.pull<value::Int>().value();
                list.get<ListV>().slice(idx1, idx2, step, elem_ti);
                m_stack.push(list);
                break;
            }

            case Opcode::ListConcat: {
                const auto& elem_ti = read_type_arg();
                auto lhs = m_stack.pull_typed(ti_list(TypeInfo(elem_ti)));
                auto rhs = m_stack.pull_typed(ti_list(TypeInfo(elem_ti)));
                lhs.get<ListV>().extend(rhs.get<ListV>(), elem_ti);
                rhs.decref();
                m_stack.push(lhs);
                break;
            }

            case Opcode::Cast: {
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
                break;
            }

            case Opcode::Invoke: {
                const auto& type_info = read_type_arg();
                cb(m_stack.pull_typed(type_info));
                break;
            }

            case Opcode::Execute: {
                auto o = m_stack.pull<value::Closure>();
                auto closure = o.closure();
                for (size_t i = closure.length(); i != 0;) {
                    m_stack.push(closure.value_at(--i));
                }
                call_fun(*o.function());
                o.decref();
                break;
            }

            case Opcode::LoadStatic: {
                auto arg = leb128_decode<Index>(it);
                const auto& o = function->module().get_value(arg);
                m_stack.push(o);
                o.incref();
                break;
            }

            case Opcode::LoadFunction: {
                auto arg = leb128_decode<Index>(it);
                auto& fn = function->module().get_function(arg);
                m_stack.push(value::Closure(fn));
                break;
            }

            case Opcode::LoadModule: {
                auto arg = leb128_decode<Index>(it);
                auto& mod = (arg == no_index)? function->module() : function->module().get_imported_module(arg);
                m_stack.push(value::Module(mod));
                break;
            }

            case Opcode::SetBase: {
                const auto level = leb128_decode<size_t>(it);
                base = m_stack.frame(m_stack.n_frames() - 1 - level).base;
                break;
            }

            case Opcode::Copy: {
                const auto arg1 = leb128_decode<size_t>(it);
                const auto addr = arg1 + m_stack.to_rel(base); // arg1 + base
                const auto size = leb128_decode<size_t>(it); // arg2
                m_stack.copy(addr, size);
                break;
            }

            case Opcode::Drop: {
                const auto addr = leb128_decode<size_t>(it);
                const auto size = leb128_decode<size_t>(it);
                m_stack.drop(addr, size);
                break;
            }

            case Opcode::Swap: {
                const auto arg1 = leb128_decode<size_t>(it);
                const auto arg2 = leb128_decode<size_t>(it);
                m_stack.swap(arg1, arg2);
                break;
            }

            case Opcode::Call0:
            case Opcode::Call1:
            case Opcode::Call:
            case Opcode::TailCall0:
            case Opcode::TailCall1:
            case Opcode::TailCall: {
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
                break;
            }

            case Opcode::MakeList: {
                const auto num_elems = leb128_decode<uint32_t>(it);
                const auto& elem_ti = read_type_arg();
                // move list contents from stack to heap
                ListV list(num_elems, elem_ti, m_stack.data());
                m_stack.drop(0, num_elems * elem_ti.size());
                // push list handle back to stack
                m_stack.push(Value{std::move(list)});
                break;
            }

            case Opcode::MakeClosure: {
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
                break;
            }

            case Opcode::IncRef: {
                auto arg = leb128_decode<Stack::StackRel>(it);
                const HeapSlot slot {static_cast<byte*>(m_stack.get_ptr(arg))};
                slot.incref();
                break;
            }

            case Opcode::DecRef: {
                auto arg = leb128_decode<Stack::StackRel>(it);
                const HeapSlot slot {static_cast<byte*>(m_stack.get_ptr(arg))};
                if (slot.decref())
                    m_stack.clear_ptr(arg);  // without this, stack dump would read after use
                break;
            }

            case Opcode::Jump: {
                auto arg = *it++;
                it += arg;
                break;
            }

            case Opcode::JumpIfNot: {
                auto arg = *it++;
                auto cond = m_stack.pull<value::Bool>();
                if (!cond.value()) {
                    it += arg;
                }
                break;
            }

            default:
                throw not_implemented(format("opcode {}", opcode));
        }
    }
}


} // namespace xci::script
