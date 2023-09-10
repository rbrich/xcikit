// Builtin.h created on 2019-05-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_BUILTIN_H
#define XCI_SCRIPT_BUILTIN_H

#include "Value.h"
#include "Module.h"
#include "ast/AST.h"
#include "Code.h"
#include <cmath>

namespace xci::script {


namespace builtin {

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
                std::pow(double(lhs), double(rhs)); }
};

const char* op_to_function_name(ast::Operator::Op op);

} // namespace builtin


class BuiltinModule : public Module {
public:
    explicit BuiltinModule(ModuleManager& module_manager);

private:
    void add_intrinsics();
    void add_types();
    void add_string_functions();
    void add_io_functions();
    void add_introspections();
};

} // namespace xci::script

#endif // include guard
