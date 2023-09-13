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
#include <limits>

namespace xci::script {


namespace builtin {

// Safe shift_left operator - shifting by too many bits gives 0
struct shift_left {
    template<class T>
    constexpr T operator()( T&& lhs, uint8_t rhs ) const noexcept {
        if (rhs >= std::numeric_limits<xci::make_unsigned_t<T>>::digits)
             return T(0);
        return std::forward<T>(lhs) << rhs;
    }
};

// Safe shift_right operator - shifting by too many bits gives 0 for unsigned
// or signed positive, -1 for signed negative (infinite sign extension).
struct shift_right {
    template<class T>
    constexpr T operator()( T&& lhs, uint8_t rhs ) const noexcept {
        if (rhs >= std::numeric_limits<xci::make_unsigned_t<T>>::digits) {
             if constexpr (std::is_signed_v<T>) {
                return lhs < 0 ? T(-1) : T(0);
             } else {
                return T(0);
             }
        }
        return std::forward<T>(lhs) >> rhs;
    }
};

// exp operator is missing in <functional>
struct exp {
    template<class T, class U>
    constexpr auto operator()( T&& lhs, U&& rhs ) const
    noexcept(noexcept(std::forward<T>(lhs) + std::forward<U>(rhs)))
    -> decltype(std::forward<T>(lhs) + std::forward<U>(rhs))
    {
        using R = decltype(std::forward<T>(lhs) + std::forward<U>(rhs));
#if defined(_MSC_VER)
        // _Signed128 / _Unsigned128 don't support cast to double
        if constexpr (!std::is_floating_point_v<T> || !std::is_floating_point_v<U>)
            return (R)(int64_t) std::pow(double(int64_t(lhs)), double(int64_t(rhs)));
        else
#endif
            return (R) std::pow(double(lhs), double(rhs));
    }
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
