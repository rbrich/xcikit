// memoization.h created on 2020-10-17 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

/// https://en.wikibooks.org/wiki/Optimizing_C%2B%2B/General_optimization_techniques/Memoization

#ifndef XCI_CORE_MEMOIZATION_H
#define XCI_CORE_MEMOIZATION_H

#include <tuple>
#include <xci/core/template/ToFunctionPtr.h>

namespace xci::core {


/// Represents memoized function. Stores the function itself
/// and `n` cells for memoization of different function args.
/// Do not construct directly, use memoize() function instead.
template<unsigned n, typename FPtr, typename Signature>
class Memoized;

template <unsigned n, typename FPtr, typename Ret, typename... Args>
class Memoized<n, FPtr, Ret(*)(Args...)> {
    using FunctionPointer = typename FPtr::Type;
    using ArgsTuple = std::tuple<Args...>;
    using ArgsStorage = typename std::aligned_storage<sizeof(ArgsTuple), alignof(ArgsTuple)>::type;

public:
    explicit Memoized(FunctionPointer func): func(func) {}

    Ret operator() (Args... args)
    {
        // On first call:
        // - last_written > valid ==> nothing memoized yet
        // - new item is written to field #0
        // On following calls:
        // - check recent items (backwards)
        // - write new items to uninitialized fields (forwards)
        // - until all items are overwritten, lookup always ends in field #0

        ArgsTuple args_tuple{args...};
        int i = last_written;
        while (i <= valid) {
            if (std::memcmp(&memo_args[i], &args_tuple, sizeof(ArgsTuple)) == 0)
                return memo_result[i];
            i = (i - 1 + n) % n;
            if (i == last_written)
                break;  // cycled through the whole cache
        }

        i = (last_written + 1) % n;
        last_written = i;
        if (valid < i)
            valid = i;
        std::memcpy(&memo_args[i], &args_tuple, sizeof(ArgsTuple));
        return memo_result[i] = func(args...);
    }

private:
    FunctionPointer func;
    ArgsStorage memo_args[n];   // store and compare as bit image, no actual objects are constructed here
    Ret memo_result[n];
    int last_written = n - 1;
    int valid = -1;
};


/// Make function object for memoization of `func`.
/// \tparam n       How many most-recent results should be remembered.
/// \param func     The function to be memoized and called by Memoized function object.
/// TODO: Restrict to trivial types in arguments
///       They are compared bit-by-bit, so it won't work for std::string etc.
///       Pointers are also quite dangerous, because the value can be recycled
template<unsigned n = 8, typename Func>
auto memoize(Func&& func) {
    auto fptr = ToFunctionPtr(std::forward<Func>(func));
    using FPtr = decltype(fptr);
    return Memoized<n, FPtr, typename FPtr::Type>{fptr.ptr};
}


} // namespace xci::core

#endif // include guard
