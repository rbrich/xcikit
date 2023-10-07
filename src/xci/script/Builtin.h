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

// Safe add with overflow check
template<class T>
constexpr T add(const T& lhs, const T& rhs ) {
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

template<class T>
constexpr T sub(const T& lhs, const T& rhs ) {
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

template<class T>
constexpr T mul(const T& lhs, const T& rhs ) {
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

template<class T>
constexpr T div(const T& lhs, const T& rhs ) {
    if constexpr (std::is_integral_v<T>) {
         if (rhs == 0)
            throw value_out_of_range("Division by zero");
         return lhs / rhs;
    } else
         return lhs / rhs;
}

template<class T>
constexpr T mod(const T& lhs, const T& rhs ) {
    if constexpr (std::is_integral_v<T> || std::is_same_v<T, uint128> || std::is_same_v<T, int128>) {
         if (rhs == 0)
            throw value_out_of_range("Division by zero");
         return lhs % rhs;
    } else if constexpr (std::is_same_v<T, float>) {
         return fmodf(lhs, rhs);
    } else if constexpr (std::is_same_v<T, double>) {
         return fmod(lhs, rhs);
    } else if constexpr (std::is_same_v<T, long double> || std::is_same_v<T, float128>) {
         return fmodl(lhs, rhs);
    }
}

template<class T>
constexpr T unsafe_mod(const T& lhs, const T& rhs ) noexcept {
    if constexpr (std::is_integral_v<T> || std::is_same_v<T, uint128> || std::is_same_v<T, int128>) {
        return lhs % rhs;
    } else if constexpr (std::is_same_v<T, float>) {
        return fmodf(lhs, rhs);
    } else if constexpr (std::is_same_v<T, double>) {
        return fmod(lhs, rhs);
    } else if constexpr (std::is_same_v<T, long double> || std::is_same_v<T, float128>) {
        return fmodl(lhs, rhs);
    }
}

template<class T>
constexpr T exp(T x, T n ) noexcept
{
    if constexpr (std::is_integral_v<T> || std::is_same_v<T, uint128> || std::is_same_v<T, int128>) {
         // https://en.wikipedia.org/wiki/Exponentiation_by_squaring
         if (n == 0)
            return 1;
         if (n < 0) {
            x = 1 / x;
            n = -n;
         }
         T res = 1;
         while (n > 1) {
            if (n & 1)
                res *= x;
            x *= x;
            n >>= 1;
         }
         return res * x;
    } else if constexpr (std::is_same_v<T, float>) {
         return powf(x, n);
    } else if constexpr (std::is_same_v<T, double>) {
         return pow(x, n);
    } else if constexpr (std::is_same_v<T, long double> || std::is_same_v<T, float128>) {
         return powl(x, n);
    }
}

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
        const auto id = intern(name);
        const auto index = add_type(TypeInfo{id, std::move(ti)});
        symtab().add({id, Symbol::TypeName, index});
        return index;
    }

    void add_symbol(const char* name, Symbol::Type type, Index idx = no_index) {
        symtab().add({intern(name), type, idx});
    }
};

} // namespace xci::script

#endif // include guard
