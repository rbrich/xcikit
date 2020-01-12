// NativeDelegate.h created on 2020-01-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_NATIVE_DELEGATE_H
#define XCI_SCRIPT_NATIVE_DELEGATE_H

#include <tuple>
#include "Value.h"
#include "Stack.h"

namespace xci::script {


/// Delegate for native functions
/// The native function must operate according to calling convention:
/// - read args from stack (first arg is on top)
/// - push return value on stack
/// The types of args and return value are specified in function signature.
/// Failing to read/write exact number of bytes may lead to hard-to-track errors.
///
/// See `native::AutoWrap` template below, which can generate the delegate
/// from C functions and lambdas.

class NativeDelegate final {
public:
    using WrapperFunction = void (*)(Stack& stack, void* data_1, void* data_2);

    NativeDelegate(WrapperFunction func)
        : m_func(func), m_data_1(nullptr), m_data_2(nullptr) {}
    NativeDelegate(WrapperFunction func, void* data_1)
            : m_func(func), m_data_1(data_1), m_data_2(nullptr) {}
    NativeDelegate(WrapperFunction func, void* data_1, void* data_2)
            : m_func(func), m_data_1(data_1), m_data_2(data_2) {}

    bool operator==(const NativeDelegate& rhs) const {
        return m_func == rhs.m_func &&
               m_data_1 == rhs.m_data_1 &&
               m_data_2 == rhs.m_data_2;
    }
    bool operator!=(const NativeDelegate& rhs) const { return !(rhs == *this); }

    void operator()(Stack& stack) const {
        return m_func(stack, m_data_1, m_data_2);
    }

private:
    WrapperFunction m_func;  // function that operates on stack, may call wrapped function
    void* m_data_1;    // may be used to store wrapped function pointer
    void* m_data_2;    // may be used for this pointer when the wrapped function is a method
};


namespace native {


/// Convert native type to `script::TypeInfo`:
///
///     auto type_info = make_type_info<int>();
///     auto params = make_type_info_list<int, string>();
///

template<class T>
typename std::enable_if_t<std::is_same_v<T, void>, TypeInfo>
make_type_info() { return TypeInfo{Type::Void}; }

template<class T>
typename std::enable_if_t<std::is_same_v<T, std::string>, TypeInfo>
make_type_info() { return TypeInfo{Type::String}; }

template<class T>
typename std::enable_if_t<std::is_integral_v<T> && sizeof(T) == 4, TypeInfo>
make_type_info() { return TypeInfo{Type::Int32}; }


/// Convert native type to `script::Value` subtype:
///
///     using T = ValueType<std::string>;
///

template<class T, class U = void>
struct ValueType_s;

template<>
struct ValueType_s<void> {
    using type = value::Void;
};

template<>
struct ValueType_s<std::string> {
    using type = value::String;
};

template<class T>
struct ValueType_s<T, typename std::enable_if_t<std::is_integral_v<T> && sizeof(T) == 4>> {
    using type = value::Int32;
};

template<class T>
using ValueType = typename ValueType_s<T>::type;


/// Call .decref() method on all values in a tuple.
template<class Tuple, std::size_t... Is>
void decref_each(Tuple&& t, std::index_sequence<Is...>)
{
    auto l = { 0, (std::get<Is>(t).decref(), 0)... };
    (void) l;
}


/// AutoWrap - generate NativeDelegate from a C++ callable
///
///     auto s = native::AutoWrap{std::forward<F>(f)};
///     fn->signature().params = s.param_types();
///     fn->signature().return_type = s.return_type();
///     fn->set_native(s.native_wrapper());
///

template<class FPtr, class Signature, class Arg0>
struct AutoWrap;

template <class FPtr, class Ret, class... Args>
struct AutoWrap<FPtr, Ret(*)(Args...), void> {
    using FunctionPointer = typename FPtr::Type;

    void* _fun_ptr;

    constexpr AutoWrap(FPtr&& f) noexcept
            : _fun_ptr(reinterpret_cast<void*>(f.ptr)) {}

    TypeInfo return_type() { return make_type_info<Ret>(); }
    std::vector<TypeInfo> param_types() { return {make_type_info<Args>()...}; }

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


template <class FPtr, class Ret, class Arg0, class... Args>
struct AutoWrap<FPtr, Ret(*)(Arg0, Args...), Arg0> {
    using FunctionPointer = typename FPtr::Type;

    void* _fun_ptr;
    void* _arg0;

    constexpr AutoWrap(FPtr&& f, Arg0 arg0) noexcept
        : _fun_ptr(reinterpret_cast<void*>(f.ptr)),
          _arg0(static_cast<void*>(arg0)) {}

    TypeInfo return_type() { return make_type_info<Ret>(); }
    std::vector<TypeInfo> param_types() { return {make_type_info<Args>()...}; }

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


template <class Callable, class FType>
struct ToFunctionPtr {
    using Type = FType;
    FType ptr;
    ToFunctionPtr(Callable&& f) : ptr(static_cast<FType>(f)) {}
};


// *** deduction guides ***

template <class FPtr> AutoWrap(FPtr&& fp)
    -> AutoWrap<FPtr, typename FPtr::Type, void>;
template <class FPtr, class TArg> AutoWrap(FPtr&& fp, TArg)
    -> AutoWrap<FPtr, typename FPtr::Type, TArg>;

// free function
template <class Ret, class... Args> ToFunctionPtr(Ret (*f)(Args...))
    -> ToFunctionPtr<decltype(f), Ret (*)(Args...)>;

// member function (just for lambda signature, members are not supported by AutoWrap)
template <class Ret, class Cls, class... Args> ToFunctionPtr(Ret (Cls::*f)(Args...))
    -> ToFunctionPtr<decltype(f), Ret (*)(Args...)>;
template <class Ret, class Cls, class... Args> ToFunctionPtr(Ret (Cls::*f)(Args...) const)
    -> ToFunctionPtr<decltype(f), Ret (*)(Args...)>;

// generic callable -> obtain function type from operator()
template <class Fun, class X = decltype(ToFunctionPtr{&std::decay_t<Fun>::operator()})> ToFunctionPtr(Fun&&)
    -> ToFunctionPtr<Fun, typename X::Type>;


} // namespace native

} // namespace xci::script

#endif // include guard
