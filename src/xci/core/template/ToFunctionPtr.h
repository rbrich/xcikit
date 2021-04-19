// ToFunctionPtr.h created on 2020-10-28 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_TOFUNCTIONPTR_H
#define XCI_CORE_TOFUNCTIONPTR_H

#include <type_traits>

namespace xci::core {


/// This template together with its deduction guides can be used to force conversion
/// of lambda object to a function pointer. It also remembers the function signature,
/// which is then passed to in ::Type member. The function pointer itself is stored in ::ptr.

template <class Callable, class FType>
struct ToFunctionPtr {
    using Type = FType;
    const FType ptr;
    constexpr explicit ToFunctionPtr(Callable&& f) : ptr(static_cast<FType>(f)) {}
};

// free function
template <class Ret, class... Args> ToFunctionPtr(Ret (*f)(Args...))
-> ToFunctionPtr<decltype(f), Ret (*)(Args...)>;

// member function (just for lambda signature,
// members are not otherwise supported by ToFunctionPtr)
template <class Ret, class Cls, class... Args> ToFunctionPtr(Ret (Cls::*f)(Args...))
-> ToFunctionPtr<decltype(f), Ret (*)(Args...)>;
template <class Ret, class Cls, class... Args> ToFunctionPtr(Ret (Cls::*f)(Args...) const)
-> ToFunctionPtr<decltype(f), Ret (*)(Args...)>;

// generic callable -> obtain function type from operator()
// (the callable must be convertible to plain function pointer,
// method pointer won't work)
template <class Fun, class X = decltype(ToFunctionPtr{&std::decay_t<Fun>::operator()})> ToFunctionPtr(Fun&&)
-> ToFunctionPtr<Fun, typename X::Type>;


} // namespace xci::core

#endif // include guard
