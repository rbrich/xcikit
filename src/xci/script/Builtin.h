// Builtin.h created on 2019-05-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_BUILTIN_H
#define XCI_SCRIPT_BUILTIN_H

#include "Value.h"
#include "Module.h"
#include "ast/AST.h"
#include "Code.h"
#include <functional>

namespace xci::script {


namespace builtin {

template <class T, class R=T> using BinaryFunction = std::function<R(T, T)>;
template <class T> using UnaryFunction = std::function<T(T)>;

BinaryFunction<value::Bool> logical_op_function(Opcode opcode);
template <class T> BinaryFunction<T> binary_op_function(Opcode opcode);

// shift_left operator is missing in <functional>
struct shift_left {
    template<class T, class U>
    constexpr auto operator()( T&& lhs, U&& rhs ) const
    noexcept(noexcept(std::forward<T>(lhs) + std::forward<U>(rhs)))
    -> decltype(std::forward<T>(lhs) + std::forward<U>(rhs))
    { return std::forward<T>(lhs) << std::forward<U>(rhs); }
};

// shift_right operator is missing in <functional>
struct shift_right {
    template<class T, class U>
    constexpr auto operator()( T&& lhs, U&& rhs ) const
    noexcept(noexcept(std::forward<T>(lhs) + std::forward<U>(rhs)))
    -> decltype(std::forward<T>(lhs) + std::forward<U>(rhs))
    { return std::forward<T>(lhs) >> std::forward<U>(rhs); }
};

// exp operator is missing in <functional>
struct exp {
    template<class T, class U>
    constexpr auto operator()( T&& lhs, U&& rhs ) const
    noexcept(noexcept(std::forward<T>(lhs) + std::forward<U>(rhs)))
    -> decltype(std::forward<T>(lhs) + std::forward<U>(rhs))
    { return (decltype(std::forward<T>(lhs) + std::forward<U>(rhs)))
                std::pow(std::forward<T>(lhs), std::forward<U>(rhs)); }
};

UnaryFunction<value::Bool> logical_not_function();
template <class T> UnaryFunction<T> unary_op_function(Opcode opcode);

const char* op_to_function_name(ast::Operator::Op op);

} // namespace builtin


class BuiltinModule : public Module {
public:
    BuiltinModule();

    static BuiltinModule& static_instance();

private:
    void add_intrinsics();
    void add_types();
    void add_io_functions();
    void add_introspections();
};

} // namespace xci::script

#endif // include guard
