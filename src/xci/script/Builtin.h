// Builtin.h created on 2019-05-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_BUILTIN_H
#define XCI_SCRIPT_BUILTIN_H

#include "Value.h"
#include "Module.h"
#include "Error.h"
#include "ast/AST.h"
#include "Code.h"
#include <cmath>
#include <limits>

namespace xci::script {


namespace builtin {


// Safe shift_left operator - shifting by too many bits gives 0
template<class T>
constexpr T shift_left( T&& lhs, uint8_t rhs ) noexcept {
    if (rhs >= std::numeric_limits<xci::make_unsigned_t<T>>::digits)
        return T(0);
    return std::forward<T>(lhs) << rhs;
}

// Safe shift_right operator - shifting by too many bits gives 0 for unsigned
// or signed positive, -1 for signed negative (infinite sign extension).
template<class T>
constexpr T shift_right( T&& lhs, uint8_t rhs ) noexcept {
    if (rhs >= std::numeric_limits<xci::make_unsigned_t<T>>::digits) {
        if constexpr (std::is_signed_v<T>) {
            return lhs < 0 ? T(-1) : T(0);
        } else {
            return T(0);
        }
    }
    return std::forward<T>(lhs) >> rhs;
}

// Similar to std::plus, but with a single type (especially the return type is not promoted)
struct Add {
    template<class T>
    constexpr T operator()(const T& lhs, const T& rhs ) const { return lhs + rhs; }
};

struct Sub {
    template<class T>
    constexpr T operator()(const T& lhs, const T& rhs ) const { return lhs - rhs; }
};

struct Mul {
    template<class T>
    constexpr T operator()(const T& lhs, const T& rhs ) const { return lhs * rhs; }
};

struct Div {
    template<class T>
    constexpr T operator()(const T& lhs, const T& rhs ) const { return lhs / rhs; }
};

struct Mod {
    template<class T>
    constexpr T operator()(const T& lhs, const T& rhs ) const {
        if constexpr (std::is_integral_v<T> || std::is_same_v<T, uint128> || std::is_same_v<T, int128>) {
            return lhs % rhs;
        } else
            return fmod(lhs, rhs);
    }
};

// Safe add with overflow check
struct AddCk {
    template<class T>
    constexpr T operator()(const T& lhs, const T& rhs ) const {
#if defined(__GNUC__)
        if constexpr (std::is_integral_v<T>) {
            T r;
            if (__builtin_add_overflow(lhs, rhs, &r))
                throw value_out_of_range("Integer overflow");
            return r;
        } else
#endif
             return lhs + rhs;
    }
};

struct SubCk {
    template<class T>
    constexpr T operator()(const T& lhs, const T& rhs ) const {
#if defined(__GNUC__)
        if constexpr (std::is_integral_v<T>) {
             T r;
             if (__builtin_sub_overflow(lhs, rhs, &r))
                throw value_out_of_range("Integer overflow");
             return r;
        } else
#endif
             return lhs - rhs;
    }
};

struct MulCk {
    template<class T>
    constexpr T operator()(const T& lhs, const T& rhs ) const {
#if defined(__GNUC__)
        // Note: fails to link with int128 on Linux with Clang (undefined reference to `__muloti4')
        if constexpr (std::is_integral_v<T> && sizeof(T) <= 8) {
             T r;
             if (__builtin_mul_overflow(lhs, rhs, &r))
                throw value_out_of_range("Integer overflow");
             return r;
        } else
#endif
             return lhs * rhs;
    }
};

struct DivCk {
    template<class T>
    constexpr T operator()(const T& lhs, const T& rhs ) const {
        if constexpr (std::is_integral_v<T>) {
             if (rhs == 0)
                throw value_out_of_range("Division by zero");
             return lhs / rhs;
        } else
             return lhs / rhs;
    }
};

// exp operator is missing in <functional>
struct Exp {
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

    Index add_named_type(const char* name, TypeInfo&& ti) {
        const auto index = add_type(TypeInfo{name, std::move(ti)});
        symtab().add({name, Symbol::TypeName, index});
        return index;
    }
};

} // namespace xci::script

#endif // include guard
