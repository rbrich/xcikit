// Machine.cpp created on 2019-05-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Machine.h"
#include "Builtin.h"
#include "Value.h"
#include "Error.h"
#include "dump.h"
#include <xci/data/coding/leb128.h>

#include <fmt/core.h>
#include <range/v3/view/reverse.hpp>
#include <cassert>
#include <functional>

namespace xci::script {

using xci::data::decode_leb128;
using ranges::cpp20::views::reverse;
using std::move;
using fmt::format;


void Machine::call(const Function& function, const InvokeCallback& cb)
{
    const Function* cur_fun = &function;
    auto it = function.code().begin();
    auto base = m_stack.size();
    auto call_fun = [this, &cur_fun, &it, &base](const Function& fn) {
        if (fn.is_native()) {
            fn.call_native(m_stack);
            return;
        }
        assert(fn.is_compiled());
        m_stack.push_frame(cur_fun, it - cur_fun->code().begin());
        cur_fun = &fn;
        it = cur_fun->code().begin();
        base = m_stack.frame().base;
        if (m_call_enter_cb)
            m_call_enter_cb(*cur_fun);
    };

    // read type argument as generated by intrinsic: `__type_id:T`
    auto read_type_arg = [&it, &cur_fun]() -> const TypeInfo& {
        // LEB128 encoding of a type:
        // 0-31 primitive types
        // 32-N current module (type index + 32)
        auto index = decode_leb128<Index>(it);
        if (index < 32) {
            // builtin module
            return cur_fun->module().get_imported_module(0).get_type(index);
        }
        return cur_fun->module().get_type(index - 32);
    };

    // Run function code
    m_stack.push_frame(nullptr, 0);
    if (m_call_enter_cb)
        m_call_enter_cb(*cur_fun);
    for (;;) {
        if (it == cur_fun->code().end()) {
            // return from function
            if (m_bytecode_trace_cb)
                m_bytecode_trace_cb(*cur_fun, cur_fun->code().end());

            if (m_call_exit_cb)
                m_call_exit_cb(*cur_fun);

            // no more stack frames?
            if (m_stack.frame().function == nullptr) {
                m_stack.pop_frame();
                assert(m_stack.size() == function.effective_return_type().size());
                break;
            }

            // return into previous call location
            cur_fun = m_stack.frame().function;
            it = cur_fun->code().begin() + m_stack.frame().instruction;
            m_stack.pop_frame();
            base = m_stack.frame().base;
            continue;
        }

        if (m_bytecode_trace_cb)
            m_bytecode_trace_cb(*cur_fun, it);


        auto opcode = static_cast<Opcode>(*it++);
        switch (opcode) {
            case Opcode::Noop:
                break;

            case Opcode::LogicalOr:
            case Opcode::LogicalAnd: {
                auto fn = builtin::logical_op_function(opcode);
                if (!fn)
                    throw NotImplemented(format("logical operator {}", opcode));
                auto lhs = m_stack.pull<value::Bool>();
                auto rhs = m_stack.pull<value::Bool>();
                m_stack.push(fn(lhs, rhs));
                break;
            }

            case Opcode::Equal_8:
            case Opcode::NotEqual_8:
            case Opcode::LessEqual_8:
            case Opcode::GreaterEqual_8:
            case Opcode::LessThan_8:
            case Opcode::GreaterThan_8: {
                auto fn = builtin::comparison_op_function<value::Byte>(opcode);
                if (!fn)
                    throw NotImplemented(format("comparison operator {}", opcode));
                auto lhs = m_stack.pull<value::Byte>();
                auto rhs = m_stack.pull<value::Byte>();
                m_stack.push(fn(lhs, rhs));
                break;
            }

            case Opcode::Equal_32:
            case Opcode::NotEqual_32:
            case Opcode::LessEqual_32:
            case Opcode::GreaterEqual_32:
            case Opcode::LessThan_32:
            case Opcode::GreaterThan_32: {
                auto fn = builtin::comparison_op_function<value::Int32>(opcode);
                if (!fn)
                    throw NotImplemented(format("comparison operator {}", opcode));
                auto lhs = m_stack.pull<value::Int32>();
                auto rhs = m_stack.pull<value::Int32>();
                m_stack.push(fn(lhs, rhs));
                break;
            }

            case Opcode::Equal_64:
            case Opcode::NotEqual_64:
            case Opcode::LessEqual_64:
            case Opcode::GreaterEqual_64:
            case Opcode::LessThan_64:
            case Opcode::GreaterThan_64: {
                auto fn = builtin::comparison_op_function<value::Int64>(opcode);
                if (!fn)
                    throw NotImplemented(format("comparison operator {}", opcode));
                auto lhs = m_stack.pull<value::Int64>();
                auto rhs = m_stack.pull<value::Int64>();
                m_stack.push(fn(lhs, rhs));
                break;
            }

            case Opcode::BitwiseOr_8:
            case Opcode::BitwiseAnd_8:
            case Opcode::BitwiseXor_8:
            case Opcode::ShiftLeft_8:
            case Opcode::ShiftRight_8:
            case Opcode::Add_8:
            case Opcode::Sub_8:
            case Opcode::Mul_8:
            case Opcode::Div_8:
            case Opcode::Mod_8:
            case Opcode::Exp_8: {
                auto fn = builtin::binary_op_function<value::Byte>(opcode);
                if (!fn)
                    throw NotImplemented(format("binary operator {}", opcode));
                auto lhs = m_stack.pull<value::Byte>();
                auto rhs = m_stack.pull<value::Byte>();
                m_stack.push(fn(lhs, rhs));
                break;
            }

            case Opcode::BitwiseOr_32:
            case Opcode::BitwiseAnd_32:
            case Opcode::BitwiseXor_32:
            case Opcode::ShiftLeft_32:
            case Opcode::ShiftRight_32:
            case Opcode::Add_32:
            case Opcode::Sub_32:
            case Opcode::Mul_32:
            case Opcode::Div_32:
            case Opcode::Mod_32:
            case Opcode::Exp_32: {
                auto fn = builtin::binary_op_function<value::Int32>(opcode);
                if (!fn)
                    throw NotImplemented(format("binary operator {}", opcode));
                auto lhs = m_stack.pull<value::Int32>();
                auto rhs = m_stack.pull<value::Int32>();
                m_stack.push(fn(lhs, rhs));
                break;
            }

            case Opcode::BitwiseOr_64:
            case Opcode::BitwiseAnd_64:
            case Opcode::BitwiseXor_64:
            case Opcode::ShiftLeft_64:
            case Opcode::ShiftRight_64:
            case Opcode::Add_64:
            case Opcode::Sub_64:
            case Opcode::Mul_64:
            case Opcode::Div_64:
            case Opcode::Mod_64:
            case Opcode::Exp_64: {
                auto fn = builtin::binary_op_function<value::Int64>(opcode);
                if (!fn)
                    throw NotImplemented(format("binary operator {}", opcode));
                auto lhs = m_stack.pull<value::Int64>();
                auto rhs = m_stack.pull<value::Int64>();
                m_stack.push(fn(lhs, rhs));
                break;
            }

            case Opcode::LogicalNot: {
                auto fn = builtin::logical_not_function();
                assert(fn);
                auto rhs = m_stack.pull<value::Bool>();
                m_stack.push(fn(rhs));
                break;
            }

            case Opcode::BitwiseNot_8:
            case Opcode::Neg_8: {
                auto fn = builtin::unary_op_function<value::Byte>(opcode);
                if (!fn)
                    throw NotImplemented(format("unary operator {}", opcode));
                auto rhs = m_stack.pull<value::Byte>();
                m_stack.push(fn(rhs));
                break;
            }

            case Opcode::BitwiseNot_32:
            case Opcode::Neg_32: {
                auto fn = builtin::unary_op_function<value::Int32>(opcode);
                if (!fn)
                    throw NotImplemented(format("unary operator {}", opcode));
                auto rhs = m_stack.pull<value::Int32>();
                m_stack.push(fn(rhs));
                break;
            }

            case Opcode::BitwiseNot_64:
            case Opcode::Neg_64: {
                auto fn = builtin::unary_op_function<value::Int64>(opcode);
                if (!fn)
                    throw NotImplemented(format("unary operator {}", opcode));
                auto rhs = m_stack.pull<value::Int64>();
                m_stack.push(fn(rhs));
                break;
            }

            case Opcode::Subscript: {
                const auto& elem_ti = read_type_arg();
                auto lhs = m_stack.pull_typed(ti_list(TypeInfo(elem_ti)));
                auto rhs = m_stack.pull<value::Int32>();
                auto idx = rhs.value();
                auto len = lhs.get<ListV>().length();
                if (idx < 0)
                    idx += (int) len;
                if (idx < 0 || (size_t) idx >= len) {
                    lhs.decref();
                    throw IndexOutOfBounds(idx, len);
                }
                Value item = lhs.get<ListV>().value_at(idx, elem_ti);
                item.incref();
                lhs.decref();
                m_stack.push(item);
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
                    throw NotImplemented(format("cast from: {:x}", arg >> 4));
                if (to_type == Type::Unknown)
                    throw NotImplemented(format("cast to: {:x}", arg & 0xf));
                auto from = m_stack.pull(TypeInfo{from_type});
                auto to = create_value(TypeInfo{to_type});
                if (!to.cast_from(from))
                    throw NotImplemented(format("cast {} to {}",
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
                for (unsigned i = closure.length(); i != 0;) {
                    m_stack.push(closure.value_at(--i));
                }
                call_fun(*o.function());
                o.decref();
                break;
            }

            case Opcode::LoadStatic: {
                auto arg = decode_leb128<size_t>(it);
                const auto& o = cur_fun->module().get_value(arg);
                m_stack.push(o);
                o.incref();
                break;
            }

            case Opcode::LoadFunction: {
                auto arg = decode_leb128<size_t>(it);
                auto& fn = cur_fun->module().get_function(arg);
                m_stack.push(value::Closure(fn));
                break;
            }

            case Opcode::SetBase: {
                const auto level = decode_leb128<size_t>(it);
                base = m_stack.frame(m_stack.n_frames() - 1 - level).base;
                break;
            }

            case Opcode::Copy: {
                const auto arg1 = decode_leb128<size_t>(it);
                const auto addr = arg1 + m_stack.to_rel(base); // arg1 + base
                const auto size = decode_leb128<size_t>(it); // arg2
                m_stack.copy(addr, size);
                break;
            }

            case Opcode::Drop: {
                const auto addr = decode_leb128<size_t>(it);
                const auto size = decode_leb128<size_t>(it);
                m_stack.drop(addr, size);
                break;
            }

            case Opcode::Swap: {
                const auto arg1 = decode_leb128<size_t>(it);
                const auto arg2 = decode_leb128<size_t>(it);
                m_stack.swap(arg1, arg2);
                break;
            }

            case Opcode::Call0:
            case Opcode::Call1:
            case Opcode::Call: {
                // get the function's module
                Module* module;
                if (opcode == Opcode::Call0) {
                    module = &cur_fun->module();
                } else {
                    size_t idx;
                    if (opcode == Opcode::Call1) {
                        idx = 0;
                    } else {
                        // read arg1
                        idx = decode_leb128<Index>(it);
                    }
                    module = &cur_fun->module().get_imported_module(idx);
                }
                // call function from the module
                auto arg = decode_leb128<Index>(it);
                auto& fn = module->get_function(arg);
                call_fun(fn);
                break;
            }

            case Opcode::MakeList: {
                const auto num_elems = decode_leb128<uint32_t>(it);
                const auto size_of_elem = decode_leb128<uint32_t>(it);
                const size_t total_size = (size_t) num_elems * size_of_elem;
                // move list contents from stack to heap
                HeapSlot slot{total_size + sizeof(uint32_t)};
                std::memcpy(slot.data(), &num_elems, sizeof(uint32_t));
                std::memcpy(slot.data() + sizeof(uint32_t), m_stack.data(), total_size);
                m_stack.drop(0, total_size);
                // push list handle back to stack
                m_stack.push(value::List{move(slot)});
                break;
            }

            case Opcode::MakeClosure: {
                auto arg = decode_leb128<Index>(it);
                // get function
                auto& fn = cur_fun->module().get_function(arg);
                // pull nonlocals + partial args
                Values closure;
                closure.reserve(fn.nonlocals().size() + fn.partial().size());
                for (const auto & ti : fn.nonlocals()) {
                    closure.add(m_stack.pull(ti));
                }
                for (const auto & ti : fn.partial()) {
                    closure.add(m_stack.pull(ti));
                }
                // push closure
                m_stack.push(value::Closure{fn, move(closure)});
                break;
            }

            case Opcode::IncRef: {
                auto arg = decode_leb128<Stack::StackRel>(it);
                HeapSlot slot {static_cast<byte*>(m_stack.get_ptr(arg))};
                slot.incref();
                break;
            }

            case Opcode::DecRef: {
                auto arg = decode_leb128<Stack::StackRel>(it);
                HeapSlot slot {static_cast<byte*>(m_stack.get_ptr(arg))};
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
                throw NotImplemented(format("opcode {}", opcode));
        }
    }
}


} // namespace xci::script
