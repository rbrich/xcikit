// NativeDelegate.h created on 2020-01-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_NATIVE_DELEGATE_H
#define XCI_SCRIPT_NATIVE_DELEGATE_H

#include "Value.h"
#include "Stack.h"
#include <xci/core/template/ToFunctionPtr.h>
#include <tuple>
#include <string>

namespace xci::script {


/// Delegate for native functions
/// The native function must operate according to calling convention:
/// - read args from stack (first arg is on top)
/// - push return value on stack
/// The types of args and return value are specified in function signature.
/// Failing to read/write exact number of bytes may lead to hard-to-track errors.
///
/// See `AutoWrap` template below, which can generate the delegate
/// from C functions and lambdas.

class NativeDelegate final {
public:
    using WrapperFunction = void (*)(Stack& stack, void* data_1, void* data_2);

    NativeDelegate() = default;
    NativeDelegate(WrapperFunction func)
            : m_func(func) {}
    NativeDelegate(WrapperFunction func, void* data_1)
            : m_func(func), m_data_1(data_1) {}
    NativeDelegate(WrapperFunction func, void* data_1, void* data_2)
            : m_func(func), m_data_1(data_1), m_data_2(data_2) {}

    bool operator==(const NativeDelegate& rhs) const {
        return m_func == rhs.m_func &&
               m_data_1 == rhs.m_data_1 &&
               m_data_2 == rhs.m_data_2;
    }

    void operator()(Stack& stack) const {
        return m_func(stack, m_data_1, m_data_2);
    }

private:
    WrapperFunction m_func = nullptr;  // function that operates on stack, may call wrapped function
    void* m_data_1 = nullptr;  // may be used to store wrapped function pointer
    void* m_data_2 = nullptr;  // may be used for `this` pointer when the wrapped function is a method
};


namespace native {

template<typename T>
concept ToVoid = std::is_void_v<T>;

template<typename T>
concept ToBool = std::is_same_v<T, bool>;

template<typename T>
concept ToChar = std::is_same_v<T, char> || std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>;

template<typename T>
concept ToUInt8 = std::is_same_v<T, byte> ||
                  (std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) == 1 &&
                   !std::is_same_v<T, char> && !std::is_same_v<T, bool>);

template<typename T>
concept ToUInt16 = std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) == 2 &&
                   !std::is_same_v<T, char16_t>;

template<typename T>
concept ToUInt32 = std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) == 4 &&
                   !std::is_same_v<T, char32_t>;

template<typename T>
concept ToUInt64 = std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) == 8;

template<typename T>
concept ToUInt128 = std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) == 16;

template<typename T>
concept ToInt8 = std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) == 1 &&
                 !std::is_same_v<T, char>;

template<typename T>
concept ToInt16 = std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) == 2;

template<typename T>
concept ToInt32 = std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) == 4;

template<typename T>
concept ToInt64 = std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) == 8;

template<typename T>
concept ToInt128 = std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) == 16;

template<typename T>
concept ToFloat32 = std::is_floating_point_v<T> && sizeof(T) == 4;

template<typename T>
concept ToFloat64 = std::is_floating_point_v<T> && sizeof(T) == 8;

template<typename T>
concept ToFloat128 = std::is_floating_point_v<T> && sizeof(T) == 16;

template<typename T>
concept ToString = std::is_constructible_v<value::String, T>;

template<typename T>
concept ToModule = std::is_same_v<T, Module&>;


/// Convert native type to `script::TypeInfo`:
///
///     auto type_info = make_type_info<int>();
///     auto params = make_type_info_list<int, string>();
///

template<ToVoid T>
TypeInfo make_type_info() { return TypeInfo{Type::Tuple}; }

template<ToBool T>
TypeInfo make_type_info() { return TypeInfo{Type::Bool}; }

template<ToChar T>
TypeInfo make_type_info() { return TypeInfo{Type::Char}; }

template<ToUInt8 T>
TypeInfo make_type_info() { return TypeInfo{Type::UInt8}; }

template<ToUInt16 T>
TypeInfo make_type_info() { return TypeInfo{Type::UInt16}; }

template<ToUInt32 T>
TypeInfo make_type_info() { return TypeInfo{Type::UInt32}; }

template<ToUInt64 T>
TypeInfo make_type_info() { return TypeInfo{Type::UInt64}; }

template<ToUInt128 T>
TypeInfo make_type_info() { return TypeInfo{Type::UInt128}; }

template<ToInt8 T>
TypeInfo make_type_info() { return TypeInfo{Type::Int8}; }

template<ToInt16 T>
TypeInfo make_type_info() { return TypeInfo{Type::Int16}; }

template<ToInt32 T>
TypeInfo make_type_info() { return TypeInfo{Type::Int32}; }

template<ToInt64 T>
TypeInfo make_type_info() { return TypeInfo{Type::Int64}; }

template<ToInt128 T>
TypeInfo make_type_info() { return TypeInfo{Type::Int128}; }

template<ToFloat32 T>
TypeInfo make_type_info() { return TypeInfo{Type::Float32}; }

template<ToFloat64 T>
TypeInfo make_type_info() { return TypeInfo{Type::Float64}; }

template<ToFloat128 T>
TypeInfo make_type_info() { return TypeInfo{Type::Float128}; }

template<ToString T>
TypeInfo make_type_info() { return TypeInfo{Type::String}; }

template<ToModule T>
TypeInfo make_type_info() { return TypeInfo{Type::Module}; }


/// Convert native type to `script::Value` subtype:
///
///     using T = ValueType<std::string>;
///

template<class T>
struct ValueType_s;

template<ToVoid T>
struct ValueType_s<T> {
    using type = value::Tuple;
};

template<ToBool T>
struct ValueType_s<T> {
    using type = value::Bool;
};

template<ToChar T>
struct ValueType_s<T> {
    using type = value::Char;
};

template<ToUInt8 T>
struct ValueType_s<T> {
    using type = value::UInt8;
};

template<ToUInt16 T>
struct ValueType_s<T> {
    using type = value::UInt16;
};

template<ToUInt32 T>
struct ValueType_s<T> {
    using type = value::UInt32;
};

template<ToUInt64 T>
struct ValueType_s<T> {
    using type = value::UInt64;
};

template<ToUInt128 T>
struct ValueType_s<T> {
    using type = value::UInt128;
};

template<ToInt8 T>
struct ValueType_s<T> {
    using type = value::Int8;
};

template<ToInt16 T>
struct ValueType_s<T> {
    using type = value::Int16;
};

template<ToInt32 T>
struct ValueType_s<T> {
    using type = value::Int32;
};

template<ToInt64 T>
struct ValueType_s<T> {
    using type = value::Int64;
};

template<ToInt128 T>
struct ValueType_s<T> {
    using type = value::Int128;
};

template<ToFloat32 T>
struct ValueType_s<T> {
    using type = value::Float32;
};

template<ToFloat64 T>
struct ValueType_s<T> {
    using type = value::Float64;
};

template<ToFloat128 T>
struct ValueType_s<T> {
    using type = value::Float128;
};

template<ToString T>
struct ValueType_s<T> {
    using type = value::String;
};

template<ToModule T>
struct ValueType_s<T> {
    using type = value::Module;
};


template<class T>
using ValueType = typename ValueType_s<T>::type;


/// Call .decref() method on all values in a tuple.
template<class Tuple, std::size_t... Is>
void decref_each(Tuple&& t, std::index_sequence<Is...>)
{
    ((void) std::get<Is>(t).decref(), ...);
}


/// AutoWrap - generate NativeDelegate from a C++ callable
///
///     auto w = AutoWrap{ToFunctionPtr(std::forward<F>(f))};
///     fn->signature().params = w.param_types();
///     fn->signature().return_type = w.return_type();
///     fn->set_native(w.native_wrapper());
///
/// Primary template parameters:
/// \tparam FPtr        struct ToFunctionPtr (see the template below)
/// \tparam Signature   function pointer signature: same as `typename FPtr::Type`
///                     (the Signature is expanded using the below specializations)
/// \tparam Arg0        type of "user data" argument to be passed as first argument
///                     when calling the function pointer, or void if not used

template<class FPtr, class Signature, class Arg0>
struct AutoWrap;

// specialization for wrapping pure function - no user data argument (Arg0)
template <class FPtr, class Ret, class... Args>
struct AutoWrap<FPtr, Ret(*)(Args...), void> {
    using FunctionPointer = typename FPtr::Type;

    void* _fun_ptr;

    constexpr AutoWrap(FPtr&& f) noexcept
            : _fun_ptr(reinterpret_cast<void*>(f.ptr)) {}

    TypeInfo return_type() { return make_type_info<Ret>(); }
    TypeInfo param_type() { return ti_tuple(make_type_info<Args>()...); }

    /// Build wrapper function which reads args from stack,
    /// converts them to C types and calls original function.
    NativeDelegate native_wrapper()
    {
        return {
            [](Stack& stack, void* fun_ptr, void*) -> void {
                // *** sample of generated code in comments ***
                auto seq = std::index_sequence_for<Args...>{};
                // auto arg1 = stack.pull<value::Type>(); ...
                using ValArgs = std::tuple<ValueType<Args>...>;
                ValArgs args {stack.pull<ValueType<Args>>()...};
                // stack.push(value::Type{fun(arg1.value(), ...)});
                stack.push(call_with_value(
                        reinterpret_cast<FunctionPointer>(fun_ptr),
                        args, seq));
                // arg1.decref(); ...
                decref_each(args, seq);
            },
            _fun_ptr
        };
    }

    template<class F, class Tuple, std::size_t... Is>
    static auto call_with_value(F&& f, Tuple&& args, std::index_sequence<Is...>)
    {
        if constexpr (std::is_void_v<Ret>) {
            // special handling for void
            f(std::get<Is>(args).value()...);
            return ValueType<Ret>{};
        } else
            return ValueType<Ret>{ f(std::get<Is>(args).value()...) };
    }
};


// specialization for wrapping method-like function - it gets user data argument (Arg0)
template <class FPtr, class Ret, class Arg0, class... Args>
struct AutoWrap<FPtr, Ret(*)(Arg0, Args...), Arg0> {
    using FunctionPointer = typename FPtr::Type;

    void* _fun_ptr;
    void* _arg0;

    constexpr AutoWrap(FPtr&& f, Arg0 arg0) noexcept
        : _fun_ptr(reinterpret_cast<void*>(f.ptr)),
          _arg0(static_cast<void*>(arg0)) {}

    TypeInfo return_type() { return make_type_info<Ret>(); }
    TypeInfo param_type() { return ti_tuple(make_type_info<Args>()...); }

    /// Build wrapper function which reads args from stack,
    /// converts them to C types and calls original function.
    NativeDelegate native_wrapper()
    {
        return {
            [](Stack& stack, void* fun_ptr, void* arg0) -> void {
                // *** sample of generated code in comments ***
                auto seq = std::index_sequence_for<Args...>{};
                // auto arg1 = stack.pull<value::Type>(); ...
                using ValArgs = std::tuple<ValueType<Args>...>;
                ValArgs args {stack.pull<ValueType<Args>>()...};
                // stack.push(value::Type{fun(arg1.value(), ...)});
                stack.push(call_with_value(
                        reinterpret_cast<FunctionPointer>(fun_ptr),
                        arg0, args, seq));
                // arg1.decref(); ...
                decref_each(args, seq);
            },
            _fun_ptr,
            _arg0
        };
    }

    template<class F, class Tuple, std::size_t... Is>
    static auto call_with_value(F&& f, void* arg0, Tuple&& args, std::index_sequence<Is...>)
    {
        if constexpr (std::is_void_v<Ret>) {
            // special handling for void
            f(static_cast<Arg0>(arg0), std::get<Is>(args).value()...);
            return ValueType<Ret>{};
        } else
            return ValueType<Ret>{ f(static_cast<Arg0>(arg0), std::get<Is>(args).value()...) };
    }
};

// FPtr is instance of ToFunctionPtr template
template <class FPtr> AutoWrap(FPtr&& fp)
    -> AutoWrap<FPtr, typename FPtr::Type, void>;
template <class FPtr, class TArg> AutoWrap(FPtr&& fp, TArg)
    -> AutoWrap<FPtr, typename FPtr::Type, TArg>;


} // namespace native

} // namespace xci::script

#endif // include guard
