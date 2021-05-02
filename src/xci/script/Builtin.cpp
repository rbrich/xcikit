// Builtin.cpp created on 2019-05-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Builtin.h"
#include "Function.h"
#include "Error.h"
#include <xci/core/file.h>
#include <xci/compat/macros.h>
#include <functional>
#include <cmath>

namespace xci::script {


template <class F, class T, class R=T>
R apply_binary_op(T lhs, T rhs) {
    return static_cast<R>( F{}(lhs.value(), rhs.value()) );
}


template <class F, class T>
T apply_unary_op(T rhs) {
    return static_cast<T>( F{}(rhs.value()) );
}


struct shift_left {
    template<class T, class U>
    constexpr auto operator()( T&& lhs, U&& rhs ) const
    noexcept(noexcept(std::forward<T>(lhs) + std::forward<U>(rhs)))
    -> decltype(std::forward<T>(lhs) + std::forward<U>(rhs))
    { return std::forward<T>(lhs) << std::forward<U>(rhs); }
};


struct shift_right {
    template<class T, class U>
    constexpr auto operator()( T&& lhs, U&& rhs ) const
    noexcept(noexcept(std::forward<T>(lhs) + std::forward<U>(rhs)))
    -> decltype(std::forward<T>(lhs) + std::forward<U>(rhs))
    { return std::forward<T>(lhs) << std::forward<U>(rhs); }
};


struct exp_emul {
    template<class T, class U>
    constexpr auto operator()( T&& lhs, U&& rhs ) const
    noexcept(noexcept(std::forward<T>(lhs) + std::forward<U>(rhs)))
    -> decltype(std::forward<T>(lhs) + std::forward<U>(rhs))
    { return (decltype(std::forward<T>(lhs) + std::forward<U>(rhs)))
        std::pow(std::forward<T>(lhs), std::forward<U>(rhs)); }
};


namespace builtin {

    BinaryFunction<value::Bool> logical_op_function(Opcode opcode)
    {
        switch (opcode) {
            case Opcode::LogicalOr:     return apply_binary_op<std::logical_or<>, value::Bool>;
            case Opcode::LogicalAnd:    return apply_binary_op<std::logical_and<>, value::Bool>;
            default:                    return nullptr;
        }
    }

    template <class T>
    BinaryFunction<T, value::Bool> comparison_op_function(Opcode opcode)
    {
        switch (opcode) {
            case Opcode::Equal_8:
            case Opcode::Equal_32:
            case Opcode::Equal_64:          return apply_binary_op<std::equal_to<>, T, value::Bool>;
            case Opcode::NotEqual_8:
            case Opcode::NotEqual_32:
            case Opcode::NotEqual_64:       return apply_binary_op<std::not_equal_to<>, T, value::Bool>;
            case Opcode::LessEqual_8:
            case Opcode::LessEqual_32:
            case Opcode::LessEqual_64:      return apply_binary_op<std::less_equal<>, T, value::Bool>;
            case Opcode::GreaterEqual_8:
            case Opcode::GreaterEqual_32:
            case Opcode::GreaterEqual_64:   return apply_binary_op<std::greater_equal<>, T, value::Bool>;
            case Opcode::LessThan_8:
            case Opcode::LessThan_32:
            case Opcode::LessThan_64:       return apply_binary_op<std::less<>, T, value::Bool>;
            case Opcode::GreaterThan_8:
            case Opcode::GreaterThan_32:
            case Opcode::GreaterThan_64:    return apply_binary_op<std::greater<>, T, value::Bool>;
            default:                    return nullptr;
        }
    }

    template <class T>
    BinaryFunction<T> binary_op_function(Opcode opcode)
    {
        switch (opcode) {
            case Opcode::BitwiseOr_8:
            case Opcode::BitwiseOr_32:
            case Opcode::BitwiseOr_64:  return apply_binary_op<std::bit_or<>, T>;
            case Opcode::BitwiseAnd_8:
            case Opcode::BitwiseAnd_32:
            case Opcode::BitwiseAnd_64: return apply_binary_op<std::bit_and<>, T>;
            case Opcode::BitwiseXor_8:
            case Opcode::BitwiseXor_32:
            case Opcode::BitwiseXor_64: return apply_binary_op<std::bit_xor<>, T>;
            case Opcode::ShiftLeft_8:
            case Opcode::ShiftLeft_32:
            case Opcode::ShiftLeft_64:  return apply_binary_op<shift_left, T>;
            case Opcode::ShiftRight_8:
            case Opcode::ShiftRight_32:
            case Opcode::ShiftRight_64: return apply_binary_op<shift_right, T>;
            case Opcode::Add_8:
            case Opcode::Add_32:
            case Opcode::Add_64:        return apply_binary_op<std::plus<>, T>;
            case Opcode::Sub_8:
            case Opcode::Sub_32:
            case Opcode::Sub_64:        return apply_binary_op<std::minus<>, T>;
            case Opcode::Mul_8:
            case Opcode::Mul_32:
            case Opcode::Mul_64:        return apply_binary_op<std::multiplies<>, T>;
            case Opcode::Div_8:
            case Opcode::Div_32:
            case Opcode::Div_64:        return apply_binary_op<std::divides<>, T>;
            case Opcode::Mod_8:
            case Opcode::Mod_32:
            case Opcode::Mod_64:        return apply_binary_op<std::modulus<>, T>;
            case Opcode::Exp_8:
            case Opcode::Exp_32:
            case Opcode::Exp_64:        return apply_binary_op<exp_emul, T>;
            default:                    return nullptr;
        }
    }

    UnaryFunction<value::Bool> logical_not_function()
    {
        return apply_unary_op<std::logical_not<>, value::Bool>;
    }

    template <class T>
    UnaryFunction<T> unary_op_function(Opcode opcode)
    {
        switch (opcode) {
            case Opcode::BitwiseNot_8:
            case Opcode::BitwiseNot_32:
            case Opcode::BitwiseNot_64: return apply_unary_op<std::bit_not<>, T>;
            case Opcode::Neg_8:
            case Opcode::Neg_32:
            case Opcode::Neg_64:        return apply_unary_op<std::negate<>, T>;
            default:                    return nullptr;
        }
    }


    template BinaryFunction<value::Byte, value::Bool> comparison_op_function<value::Byte>(Opcode opcode);
    template BinaryFunction<value::Int32, value::Bool> comparison_op_function<value::Int32>(Opcode opcode);
    template BinaryFunction<value::Int64, value::Bool> comparison_op_function<value::Int64>(Opcode opcode);
    template BinaryFunction<value::Byte> binary_op_function<value::Byte>(Opcode opcode);
    template BinaryFunction<value::Int32> binary_op_function<value::Int32>(Opcode opcode);
    template BinaryFunction<value::Int64> binary_op_function<value::Int64>(Opcode opcode);
    template UnaryFunction<value::Byte> unary_op_function<value::Byte>(Opcode opcode);
    template UnaryFunction<value::Int32> unary_op_function<value::Int32>(Opcode opcode);
    template UnaryFunction<value::Int64> unary_op_function<value::Int64>(Opcode opcode);

}  // namespace builtin


const char* builtin::op_to_name(ast::Operator::Op op)
{
    using Op = ast::Operator;
    switch (op) {
        case Op::Undefined:     return nullptr;
        case Op::Comma:         return ",";
        case Op::LogicalOr:     return "||";
        case Op::LogicalAnd:    return "&&";
        case Op::Equal:         return "==";
        case Op::NotEqual:      return "!=";
        case Op::LessEqual:     return "<=";
        case Op::GreaterEqual:  return ">=";
        case Op::LessThan:      return "<";
        case Op::GreaterThan:   return ">";
        case Op::BitwiseOr:     return "|";
        case Op::BitwiseAnd:    return "&";
        case Op::BitwiseXor:    return "^";
        case Op::ShiftLeft:     return "<<";
        case Op::ShiftRight:    return ">>";
        case Op::Add:           return "+";
        case Op::Sub:           return "-";
        case Op::Mul:           return "*";
        case Op::Div:           return "/";
        case Op::Mod:           return "%";
        case Op::Exp:           return "**";
        case Op::Subscript:     return "[]";
        case Op::LogicalNot:    return "-";
        case Op::BitwiseNot:    return "~";
        case Op::UnaryPlus:     return "+";
        case Op::UnaryMinus:    return "-";
        case Op::DotCall:       return ".";
    }
    UNREACHABLE;
}


const char* builtin::op_to_function_name(ast::Operator::Op op)
{
    using Op = ast::Operator;
    switch (op) {
        case Op::Undefined:     return nullptr;
        case Op::Comma:         return nullptr;
        case Op::LogicalOr:     return "or";
        case Op::LogicalAnd:    return "and";
        case Op::Equal:         return "eq";
        case Op::NotEqual:      return "ne";
        case Op::LessEqual:     return "le";
        case Op::GreaterEqual:  return "ge";
        case Op::LessThan:      return "lt";
        case Op::GreaterThan:   return "gt";
        case Op::BitwiseOr:     return "bit_or";
        case Op::BitwiseAnd:    return "bit_and";
        case Op::BitwiseXor:    return "bit_xor";
        case Op::ShiftLeft:     return "shift_left";
        case Op::ShiftRight:    return "shift_right";
        case Op::Add:           return "add";
        case Op::Sub:           return "sub";
        case Op::Mul:           return "mul";
        case Op::Div:           return "div";
        case Op::Mod:           return "mod";
        case Op::Exp:           return "exp";
        case Op::Subscript:     return "subscript";
        case Op::LogicalNot:    return "not";
        case Op::BitwiseNot:    return "bit_not";
        case Op::UnaryMinus:    return "neg";
        case Op::UnaryPlus:     return nullptr;
        case Op::DotCall:       return nullptr;
    }
    UNREACHABLE;
}


BuiltinModule::BuiltinModule() : Module("builtin")
{
    symtab().add({"void", Symbol::Value, add_value(TypedValue{value::Void()})});
    symtab().add({"false", Symbol::Value, add_value(TypedValue{value::Bool(false)})});
    symtab().add({"true", Symbol::Value, add_value(TypedValue{value::Bool(true)})});
    add_logical_op_function("or", Opcode::LogicalOr);
    add_logical_op_function("and", Opcode::LogicalAnd);
    add_bitwise_op_function("bit_or", Opcode::BitwiseOr_8);
    add_bitwise_op_function("bit_and", Opcode::BitwiseAnd_8);
    add_bitwise_op_function("bit_xor", Opcode::BitwiseXor_8);
    add_bitwise_op_function("shift_left", Opcode::ShiftLeft_8);
    add_bitwise_op_function("shift_right", Opcode::ShiftRight_8);
    add_arithmetic_op_function("add", Opcode::Add_8);
    add_arithmetic_op_function("sub", Opcode::Sub_8);
    add_arithmetic_op_function("mul", Opcode::Mul_8);
    add_arithmetic_op_function("div", Opcode::Div_8);
    add_arithmetic_op_function("mod", Opcode::Mod_8);
    add_arithmetic_op_function("exp", Opcode::Exp_8);
    add_unary_op_functions();
    add_subscript_function();
    add_intrinsics();
    add_types();
    add_io_functions();
}

BuiltinModule& BuiltinModule::static_instance()
{
    static BuiltinModule instance;
    return instance;
}


void
BuiltinModule::add_logical_op_function(const std::string& name, Opcode opcode)
{
    // build function object
    auto fn = std::make_unique<Function>(*this, symtab().add_child(name));
    fn->signature().return_type = ti_bool();
    fn->add_parameter("lhs", ti_bool());
    fn->add_parameter("rhs", ti_bool());
    fn->set_compiled();
    fn->code().add_opcode(opcode);  // call operator
    symtab().add({name, Symbol::Function, add_function(std::move(fn))});
}


void
BuiltinModule::add_bitwise_op_function(const std::string& name, Opcode opcode)
{
    auto fn8 = std::make_unique<Function>(*this, symtab().add_child(name));
    fn8->signature().return_type = ti_byte();
    fn8->add_parameter("lhs", ti_byte());
    fn8->add_parameter("rhs", ti_byte());
    fn8->set_compiled();
    fn8->code().add_opcode(opcode);
    auto p8 = symtab().add({name, Symbol::Function, add_function(std::move(fn8))});

    auto fn32 = std::make_unique<Function>(*this, symtab().add_child(name));
    fn32->signature().return_type = ti_int32();
    fn32->add_parameter("lhs", ti_int32());
    fn32->add_parameter("rhs", ti_int32());
    fn32->set_compiled();
    fn32->code().add_opcode(opcode + 1);
    auto p32 = symtab().add({name, Symbol::Function, add_function(std::move(fn32))});

    auto fn64 = std::make_unique<Function>(*this, symtab().add_child(name));
    fn64->signature().return_type = ti_int64();
    fn64->add_parameter("lhs", ti_int64());
    fn64->add_parameter("rhs", ti_int64());
    fn64->set_compiled();
    fn64->code().add_opcode(opcode + 2);
    auto p64 = symtab().add({name, Symbol::Function, add_function(std::move(fn64))});

    p8->set_next(p32);
    p32->set_next(p64);
}


void
BuiltinModule::add_arithmetic_op_function(const std::string& name, Opcode opcode)
{
    auto fn8 = std::make_unique<Function>(*this, symtab().add_child(name));
    fn8->signature().return_type = ti_byte();
    fn8->add_parameter("lhs", ti_byte());
    fn8->add_parameter("rhs", ti_byte());
    fn8->set_compiled();
    fn8->code().add_opcode(opcode);
    auto p8 = symtab().add({name, Symbol::Function, add_function(std::move(fn8))});

    auto fn32 = std::make_unique<Function>(*this, symtab().add_child(name));
    fn32->signature().return_type = ti_int32();
    fn32->add_parameter("lhs", ti_int32());
    fn32->add_parameter("rhs", ti_int32());
    fn32->set_compiled();
    fn32->code().add_opcode(opcode + 1);
    auto p32 = symtab().add({name, Symbol::Function, add_function(std::move(fn32))});

    auto fn64 = std::make_unique<Function>(*this, symtab().add_child(name));
    fn64->signature().return_type = ti_int64();
    fn64->add_parameter("lhs", ti_int64());
    fn64->add_parameter("rhs", ti_int64());
    fn64->set_compiled();
    fn64->code().add_opcode(opcode + 2);
    auto p64 = symtab().add({name, Symbol::Function, add_function(std::move(fn64))});

    p8->set_next(p32);
    p32->set_next(p64);
}


void
BuiltinModule::add_unary_op_functions()
{
    // LogicalNot
    {
        auto name = "not";
        auto opcode = Opcode::LogicalNot;

        auto fn = std::make_unique<Function>(*this, symtab().add_child(name));
        fn->signature().return_type = ti_bool();
        fn->add_parameter("rhs", ti_bool());
        fn->set_compiled();
        fn->code().add_opcode(opcode);
        symtab().add({name, Symbol::Function, add_function(std::move(fn))});
    }

    // BitwiseNot
    {
        auto name = "bit_not";
        auto opcode = Opcode::BitwiseNot_8;

        auto fn8 = std::make_unique<Function>(*this, symtab().add_child(name));
        fn8->signature().return_type = ti_byte();
        fn8->add_parameter("rhs", ti_byte());
        fn8->set_compiled();
        fn8->code().add_opcode(opcode);
        auto p8 = symtab().add({name, Symbol::Function, add_function(std::move(fn8))});

        auto fn32 = std::make_unique<Function>(*this, symtab().add_child(name));
        fn32->signature().return_type = ti_int32();
        fn32->add_parameter("rhs", ti_int32());
        fn32->set_compiled();
        fn32->code().add_opcode(opcode + 1);
        auto p32 = symtab().add({name, Symbol::Function, add_function(std::move(fn32))});

        auto fn64 = std::make_unique<Function>(*this, symtab().add_child(name));
        fn64->signature().return_type = ti_int64();
        fn64->add_parameter("rhs", ti_int64());
        fn64->set_compiled();
        fn64->code().add_opcode(opcode + 2);
        auto p64 = symtab().add({name, Symbol::Function, add_function(std::move(fn64))});

        p8->set_next(p32);
        p32->set_next(p64);
    }

    // UnaryMinus
    {
        auto name = "neg";
        auto opcode = Opcode::Neg_8;

        auto fn8 = std::make_unique<Function>(*this, symtab().add_child(name));
        fn8->signature().return_type = ti_byte();
        fn8->add_parameter("rhs", ti_byte());
        fn8->set_compiled();
        fn8->code().add_opcode(opcode);
        auto p8 = symtab().add({name, Symbol::Function, add_function(std::move(fn8))});

        auto fn32 = std::make_unique<Function>(*this, symtab().add_child(name));
        fn32->signature().return_type = ti_int32();
        fn32->add_parameter("rhs", ti_int32());
        fn32->set_compiled();
        fn32->code().add_opcode(opcode + 1);
        auto p32 = symtab().add({name, Symbol::Function, add_function(std::move(fn32))});

        auto fn64 = std::make_unique<Function>(*this, symtab().add_child(name));
        fn64->signature().return_type = ti_int64();
        fn64->add_parameter("rhs", ti_int64());
        fn64->set_compiled();
        fn64->code().add_opcode(opcode + 2);
        auto p64 = symtab().add({name, Symbol::Function, add_function(std::move(fn64))});

        p8->set_next(p32);
        p32->set_next(p64);
    }
}


void
BuiltinModule::add_subscript_function()
{
    auto name = "subscript";

    auto fn = std::make_unique<Function>(*this, symtab().add_child(name));
    fn->signature().return_type = ti_int32();
    fn->add_parameter("lhs", ti_list(ti_int32()));
    fn->add_parameter("rhs", ti_int32());
    fn->set_compiled();
    fn->code().add_opcode(Opcode::Subscript_32);
    symtab().add({name, Symbol::Function, add_function(std::move(fn))});
}


void BuiltinModule::add_intrinsics()
{
    // no args
    symtab().add({"__noop", Symbol::Instruction, Index(Opcode::Noop)});
    symtab().add({"__logical_not", Symbol::Instruction, Index(Opcode::LogicalNot)});
    symtab().add({"__logical_or", Symbol::Instruction, Index(Opcode::LogicalOr)});
    symtab().add({"__logical_and", Symbol::Instruction, Index(Opcode::LogicalAnd)});
    symtab().add({"__equal_8", Symbol::Instruction, Index(Opcode::Equal_8)});
    symtab().add({"__equal_32", Symbol::Instruction, Index(Opcode::Equal_32)});
    symtab().add({"__equal_64", Symbol::Instruction, Index(Opcode::Equal_64)});
    symtab().add({"__not_equal_8", Symbol::Instruction, Index(Opcode::NotEqual_8)});
    symtab().add({"__not_equal_32", Symbol::Instruction, Index(Opcode::NotEqual_32)});
    symtab().add({"__not_equal_64", Symbol::Instruction, Index(Opcode::NotEqual_64)});
    symtab().add({"__less_equal_8", Symbol::Instruction, Index(Opcode::LessEqual_8)});
    symtab().add({"__less_equal_32", Symbol::Instruction, Index(Opcode::LessEqual_32)});
    symtab().add({"__less_equal_64", Symbol::Instruction, Index(Opcode::LessEqual_64)});
    symtab().add({"__greater_equal_8", Symbol::Instruction, Index(Opcode::GreaterEqual_8)});
    symtab().add({"__greater_equal_32", Symbol::Instruction, Index(Opcode::GreaterEqual_32)});
    symtab().add({"__greater_equal_64", Symbol::Instruction, Index(Opcode::GreaterEqual_64)});
    symtab().add({"__less_than_8", Symbol::Instruction, Index(Opcode::LessThan_8)});
    symtab().add({"__less_than_32", Symbol::Instruction, Index(Opcode::LessThan_32)});
    symtab().add({"__less_than_64", Symbol::Instruction, Index(Opcode::LessThan_64)});
    symtab().add({"__greater_than_8", Symbol::Instruction, Index(Opcode::GreaterThan_8)});
    symtab().add({"__greater_than_32", Symbol::Instruction, Index(Opcode::GreaterThan_32)});
    symtab().add({"__greater_than_64", Symbol::Instruction, Index(Opcode::GreaterThan_64)});
    symtab().add({"__bitwise_not_8", Symbol::Instruction, Index(Opcode::BitwiseNot_8)});
    symtab().add({"__bitwise_not_32", Symbol::Instruction, Index(Opcode::BitwiseNot_32)});
    symtab().add({"__bitwise_not_64", Symbol::Instruction, Index(Opcode::BitwiseNot_64)});
    symtab().add({"__bitwise_or_8", Symbol::Instruction, Index(Opcode::BitwiseOr_8)});
    symtab().add({"__bitwise_or_32", Symbol::Instruction, Index(Opcode::BitwiseOr_32)});
    symtab().add({"__bitwise_or_64", Symbol::Instruction, Index(Opcode::BitwiseOr_64)});
    symtab().add({"__bitwise_and_8", Symbol::Instruction, Index(Opcode::BitwiseAnd_8)});
    symtab().add({"__bitwise_and_32", Symbol::Instruction, Index(Opcode::BitwiseAnd_32)});
    symtab().add({"__bitwise_and_64", Symbol::Instruction, Index(Opcode::BitwiseAnd_64)});
    symtab().add({"__bitwise_xor_8", Symbol::Instruction, Index(Opcode::BitwiseXor_8)});
    symtab().add({"__bitwise_xor_32", Symbol::Instruction, Index(Opcode::BitwiseXor_32)});
    symtab().add({"__bitwise_xor_64", Symbol::Instruction, Index(Opcode::BitwiseXor_64)});
    symtab().add({"__shift_left_8", Symbol::Instruction, Index(Opcode::ShiftLeft_8)});
    symtab().add({"__shift_left_32", Symbol::Instruction, Index(Opcode::ShiftLeft_32)});
    symtab().add({"__shift_left_64", Symbol::Instruction, Index(Opcode::ShiftLeft_64)});
    symtab().add({"__shift_right_8", Symbol::Instruction, Index(Opcode::ShiftRight_8)});
    symtab().add({"__shift_right_32", Symbol::Instruction, Index(Opcode::ShiftRight_32)});
    symtab().add({"__shift_right_64", Symbol::Instruction, Index(Opcode::ShiftRight_64)});
    symtab().add({"__neg_8", Symbol::Instruction, Index(Opcode::Neg_8)});
    symtab().add({"__neg_32", Symbol::Instruction, Index(Opcode::Neg_32)});
    symtab().add({"__neg_64", Symbol::Instruction, Index(Opcode::Neg_64)});
    symtab().add({"__add_8", Symbol::Instruction, Index(Opcode::Add_8)});
    symtab().add({"__add_32", Symbol::Instruction, Index(Opcode::Add_32)});
    symtab().add({"__add_64", Symbol::Instruction, Index(Opcode::Add_64)});
    symtab().add({"__sub_8", Symbol::Instruction, Index(Opcode::Sub_8)});
    symtab().add({"__sub_32", Symbol::Instruction, Index(Opcode::Sub_32)});
    symtab().add({"__sub_64", Symbol::Instruction, Index(Opcode::Sub_64)});
    symtab().add({"__mul_8", Symbol::Instruction, Index(Opcode::Mul_8)});
    symtab().add({"__mul_32", Symbol::Instruction, Index(Opcode::Mul_32)});
    symtab().add({"__mul_64", Symbol::Instruction, Index(Opcode::Mul_64)});
    symtab().add({"__div_8", Symbol::Instruction, Index(Opcode::Div_8)});
    symtab().add({"__div_32", Symbol::Instruction, Index(Opcode::Div_32)});
    symtab().add({"__div_64", Symbol::Instruction, Index(Opcode::Div_64)});
    symtab().add({"__mod_8", Symbol::Instruction, Index(Opcode::Mod_8)});
    symtab().add({"__mod_32", Symbol::Instruction, Index(Opcode::Mod_32)});
    symtab().add({"__mod_64", Symbol::Instruction, Index(Opcode::Mod_64)});
    symtab().add({"__exp_8", Symbol::Instruction, Index(Opcode::Exp_8)});
    symtab().add({"__exp_32", Symbol::Instruction, Index(Opcode::Exp_32)});
    symtab().add({"__exp_64", Symbol::Instruction, Index(Opcode::Exp_64)});
    symtab().add({"__subscript_32", Symbol::Instruction, Index(Opcode::Subscript_32)});
    // one arg
    symtab().add({"__cast", Symbol::Instruction, Index(Opcode::Cast)});
    // two args
    symtab().add({"__copy", Symbol::Instruction, Index(Opcode::Copy)});
    symtab().add({"__drop", Symbol::Instruction, Index(Opcode::Drop)});
    /*
    // not yet found any use for these, uncomment when needed
    symtab().add({"__execute", Symbol::Instruction, Index(Opcode::Execute)});
    symtab().add({"__load_static", Symbol::Instruction, Index(Opcode::LoadStatic)});
    symtab().add({"__load_module", Symbol::Instruction, Index(Opcode::LoadModule)});
    symtab().add({"__load_function", Symbol::Instruction, Index(Opcode::LoadFunction)});
    symtab().add({"__call0", Symbol::Instruction, Index(Opcode::Call0)});
    symtab().add({"__call1", Symbol::Instruction, Index(Opcode::Call1)});
    symtab().add({"__partial_execute", Symbol::Instruction, Index(Opcode::PartialExecute)});
    symtab().add({"__make_closure", Symbol::Instruction, Index(Opcode::MakeClosure)});
    symtab().add({"__inc_ref", Symbol::Instruction, Index(Opcode::IncRef)});
    symtab().add({"__dec_ref", Symbol::Instruction, Index(Opcode::DecRef)});
    symtab().add({"__jump", Symbol::Instruction, Index(Opcode::Jump)});
    symtab().add({"__jump_if_not", Symbol::Instruction, Index(Opcode::JumpIfNot)});
    symtab().add({"__invoke", Symbol::Instruction, Index(Opcode::Invoke)});
    symtab().add({"__call", Symbol::Instruction, Index(Opcode::Call)});
    symtab().add({"__partial0", Symbol::Instruction, Index(Opcode::Partial0)});
    symtab().add({"__partial1", Symbol::Instruction, Index(Opcode::Partial1)});
    symtab().add({"__make_list", Symbol::Instruction, Index(Opcode::MakeList)});
    symtab().add({"__partial", Symbol::Instruction, Index(Opcode::Partial)});
    */
}


void BuiltinModule::add_types()
{
    symtab().add({"Void", Symbol::TypeName, add_type(ti_void())});
    symtab().add({"Bool", Symbol::TypeName, add_type(ti_bool())});
    symtab().add({"Byte", Symbol::TypeName, add_type(ti_byte())});
    symtab().add({"Char", Symbol::TypeName, add_type(ti_char())});
    symtab().add({"Int", Symbol::TypeName, add_type(ti_int32())});
    symtab().add({"Int32", Symbol::TypeName, add_type(ti_int32())});
    symtab().add({"Int64", Symbol::TypeName, add_type(ti_int64())});
    symtab().add({"Float", Symbol::TypeName, add_type(ti_float32())});
    symtab().add({"Float32", Symbol::TypeName, add_type(ti_float32())});
    symtab().add({"Float64", Symbol::TypeName, add_type(ti_float64())});
    symtab().add({"String", Symbol::TypeName, add_type(ti_string())});
}


static void write_bytes(Stack& stack, void*, void*)
{
    auto arg = stack.pull<value::Bytes>();
    stack.stream_out().write(arg.value());
    arg.decref();
}


static void write_string(Stack& stack, void*, void*)
{
    auto arg = stack.pull<value::String>();
    stack.stream_out().write(arg.value());
    arg.decref();
}


static void flush_out(Stack& stack, void*, void*)
{
    stack.stream_out().flush();
}


static void write_error(Stack& stack, void*, void*)
{
    auto arg = stack.pull<value::String>();
    stack.stream_err().write(arg.value());
    arg.decref();
}


static void read_string(Stack& stack, void*, void*)
{
    auto arg = stack.pull<value::Int32>();
    auto s = stack.stream_in().read(arg.value());
    stack.push(value::String(s));
}


static void open_file(Stack& stack, void*, void*)
{
    auto path = stack.pull<value::String>();
    std::string path_s {path.value()};
    path.decref();

    auto flags = stack.pull<value::String>();
    std::string flags_s {flags.value()};
    flags.decref();

    FILE* f = fopen(path_s.c_str(), flags_s.c_str());

    stack.push(value::Stream(script::Stream{script::Stream::CFile{f}}));
}


static void internal_streams(Stack& stack, void*, void*)
{
    stack.push(value::Tuple{stack.get_stream_in(), stack.get_stream_out(), stack.get_stream_err()});
}


static void output_stream_enter1(Stack& stack, void*, void*)
{
    auto out = stack.pull<value::Stream>();
    stack.swap_stream_out(out);
    stack.push(out);  // return the original stream -> pass it to leave function
}


static void output_stream_leave1(Stack& stack, void*, void*)
{
    auto out = stack.pull<value::Stream>();
    stack.swap_stream_out(out);
    out.decref();  // dispose the stream which was used inside the context
}


static void output_stream_enter2(Stack& stack, void*, void*)
{
    auto in = stack.pull<value::Stream>();
    auto out = stack.pull<value::Stream>();
    stack.swap_stream_in(in);
    stack.swap_stream_out(out);
    stack.push(value::Tuple{in, out});
}


static void output_stream_leave2(Stack& stack, void*, void*)
{
    auto in = stack.pull<value::Stream>();
    auto out = stack.pull<value::Stream>();
    stack.swap_stream_in(in);
    stack.swap_stream_out(out);
    in.decref();
    out.decref();
}


static void output_stream_enter3(Stack& stack, void*, void*)
{
    auto in = stack.pull<value::Stream>();
    auto out = stack.pull<value::Stream>();
    auto err = stack.pull<value::Stream>();
    // undef value can be passed via incomplete Streams struct - keep original stream
    if (in.value())
        stack.swap_stream_in(in);
    if (out.value())
        stack.swap_stream_out(out);
    if (err.value())
        stack.swap_stream_err(err);
    stack.push(value::Tuple{in, out, err});
}


static void output_stream_leave3(Stack& stack, void*, void*)
{
    auto in = stack.pull<value::Stream>();
    auto out = stack.pull<value::Stream>();
    auto err = stack.pull<value::Stream>();
    // undef on stack means the stream wasn't swapped - see above (enter3)
    if (in.value())
        stack.swap_stream_in(in);
    if (out.value())
        stack.swap_stream_out(out);
    if (err.value())
        stack.swap_stream_err(err);
    in.decref();
    out.decref();
    err.decref();
}


void BuiltinModule::add_io_functions()
{
    // types
    auto streams = ti_struct({
            {"in", ti_stream()},
            {"out", ti_stream()},
            {"err", ti_stream()}
    });
    symtab().add({"Streams", Symbol::TypeName, add_type(streams)});

    // values
    symtab().add({"stdin", Symbol::Value, add_value(TypedValue{value::Stream(script::Stream::default_stdin())})});
    symtab().add({"stdout", Symbol::Value, add_value(TypedValue{value::Stream(script::Stream::default_stdout())})});
    symtab().add({"stderr", Symbol::Value, add_value(TypedValue{value::Stream(script::Stream::default_stderr())})});
    symtab().add({"null", Symbol::Value, add_value(TypedValue{value::Stream(script::Stream::null())})});

    // functions
    auto ps = add_native_function("write", {ti_string()}, ti_void(), write_string);
    auto pb = add_native_function("write", {ti_bytes()}, ti_void(), write_bytes);
    ps->set_next(pb);
    add_native_function("flush", {}, ti_void(), flush_out);
    add_native_function("error", {ti_string()}, ti_void(), write_error);
    add_native_function("read", {ti_int32()}, ti_string(), read_string);
    add_native_function("open", {ti_string(), ti_string()}, ti_stream(), open_file);
    add_native_function("__streams", {}, TypeInfo(streams), internal_streams);

    auto enter1 = add_native_function("enter", {ti_stream()}, ti_stream(), output_stream_enter1);
    auto leave1 = add_native_function("leave", {ti_stream()}, ti_void(), output_stream_leave1);
    auto enter2 = add_native_function("enter", {ti_tuple(ti_stream(), ti_stream())}, ti_tuple(ti_stream(), ti_stream()), output_stream_enter2);
    auto leave2 = add_native_function("leave", {ti_tuple(ti_stream(), ti_stream())}, ti_void(), output_stream_leave2);
    auto enter3 = add_native_function("enter", {ti_tuple(ti_stream(), ti_stream(), ti_stream())}, ti_tuple(ti_stream(), ti_stream(), ti_stream()), output_stream_enter3);
    auto leave3 = add_native_function("leave", {ti_tuple(ti_stream(), ti_stream(), ti_stream())}, ti_void(), output_stream_leave3);
    auto enter_s = add_native_function("enter", {streams}, TypeInfo(streams), output_stream_enter3);
    auto leave_s = add_native_function("leave", {streams}, ti_void(), output_stream_leave3);
    enter1->set_next(enter2);
    enter2->set_next(enter3);
    enter3->set_next(enter_s);
    leave1->set_next(leave2);
    leave2->set_next(leave3);
    leave3->set_next(leave_s);
}


} // namespace xci::script
