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
    using NativeFunction = void (*)(Stack& stack, void* user_data);

    NativeDelegate(NativeFunction func)
        : m_wrapper_func(func), m_user_data(nullptr) {}
    NativeDelegate(NativeFunction func, void* user_data)
            : m_wrapper_func(func), m_user_data(user_data) {}

    bool operator==(const NativeDelegate& rhs) const {
        return m_wrapper_func == rhs.m_wrapper_func &&
               m_user_data == rhs.m_user_data;
    }
    bool operator!=(const NativeDelegate& rhs) const { return !(rhs == *this); }

    void operator()(Stack& stack) const
    {
        return m_wrapper_func(stack, m_user_data);
    }

private:
    NativeFunction m_wrapper_func;
    void* m_user_data;
};


namespace native {


/// Convert native type to `script::TypeInfo`:
///
///     auto type_info = make_type_info<int>();
///     auto params = make_type_info_list<int, string>();
///

template<class T>
typename std::enable_if_t<std::is_same_v<T, std::string>, TypeInfo>
make_type_info()
{
    return TypeInfo{Type::String};
}


template<class T>
typename std::enable_if_t<std::is_integral_v<T> && sizeof(T) == 4, TypeInfo>
make_type_info()
{
    return TypeInfo{Type::Int32};
}


/// Convert native type to `script::Value` subtype:
///
///     using T = ValueType<std::string>;
///

template<class T, class U = void>
struct ValueType_s;

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


/// AutoWrap - generate NativeDelegate from a C++ callable
///
///     auto s = native::AutoWrap{std::forward<F>(f)};
///     fn->signature().params = s.param_types();
///     fn->signature().return_type = s.return_type();
///     fn->set_native(s.native_wrapper());
///

template <class... Args> struct Pack {};

template <class F, class E = void>
struct Eraser;

template <class F>
struct Eraser<F, typename std::enable_if_t<std::is_pointer_v<F>>> {
    static void* erase_type(F&& f) { return reinterpret_cast<void*>(f); }
    static F restore_type(void* f) { return reinterpret_cast<F>(f); }
};

template <class F>
struct Eraser<F, typename std::enable_if_t<!std::is_pointer_v<F>>> {
    static void* erase_type(F&& f) { return reinterpret_cast<void*>(std::addressof(f)); }
    static F restore_type(void* f) { return *reinterpret_cast<std::add_pointer_t<F>>(f); }
};

template<class Fun, class Signature>
struct AutoWrap;

template <class Fun, class Ret, class... Args>
struct AutoWrap<Fun, Pack<Ret, Args...>> {
    using Signature = Pack<Ret, Args...>;

    void* _fun_ptr;  // type-erased function pointer

    constexpr AutoWrap(Fun&& f) noexcept
        : _fun_ptr(Eraser<Fun>::erase_type(std::forward<Fun>(f))) {}

    TypeInfo return_type()
    {
        return make_type_info<Ret>();
    }

    std::vector<TypeInfo> param_types()
    {
        return {make_type_info<Args>()...};
    }

    /// Build wrapper function which reads args from stack,
    /// converts them to C types and calls original function.
    NativeDelegate native_wrapper()
    {
        return {
            [](Stack& stack, void* fun_ptr) -> void {
                // *** sample of generated code in comments ***
                auto seq = std::index_sequence_for<Args...>{};
                // auto arg1 = stack.pull<value::Type>(); ...
                using ValArgs = std::tuple<ValueType<Args>...>;
                ValArgs args {stack.pull<ValueType<Args>>()...};
                // auto result = fun(arg1.value(), ...)
                auto result = call_with_value(
                        Eraser<Fun>::restore_type(fun_ptr), args, seq);
                // stack.push(value::Type{result});
                stack.push(ValueType<Ret>{result});
                // arg1.decref(); ...
                decref_each(args, seq);
            },
            _fun_ptr
        };
    }

    template<class F, class Tuple, std::size_t... Is>
    static auto call_with_value(F&& f, Tuple&& args, std::index_sequence<Is...>)
    {
        return f(std::get<Is>(args).value()...);
    }

    /// Call .decref() method on all values in a tuple.
    template<class Tuple, std::size_t... Is>
    static void decref_each(Tuple&& t, std::index_sequence<Is...>)
    {
        auto l = { (std::get<Is>(t).decref(), 0)... };
        (void) l;
    }
};


// *** deduction guides ***

// free function
template <class Ret, class... Args> AutoWrap(Ret (*f)(Args...))
    -> AutoWrap<Ret(*)(Args...), Pack<Ret, Args...>>;

// member function
template <class Ret, class Cls, class... Args> AutoWrap(Ret (Cls::*f)(Args...))
    -> AutoWrap<Ret (Cls::*)(Args...), Pack<Ret, Args...>>;
template <class Ret, class Cls, class... Args> AutoWrap(Ret (Cls::*f)(Args...) const)
    -> AutoWrap<Ret (Cls::*)(Args...) const, Pack<Ret, Args...>>;

// generic callable -> obtain function type from operator()
template <class Fun, class X = decltype(AutoWrap{&std::decay_t<Fun>::operator()})> AutoWrap(Fun&&)
    -> AutoWrap<Fun, typename X::Signature>;


} // namespace native

} // namespace xci::script

#endif // include guard
