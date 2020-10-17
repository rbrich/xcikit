// memoization.h created on 2020-10-17 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

/// https://en.wikibooks.org/wiki/Optimizing_C%2B%2B/General_optimization_techniques/Memoization

#ifndef XCI_CORE_MEMOIZATION_H
#define XCI_CORE_MEMOIZATION_H

#include <tuple>

namespace xci::core {


/// Represents memoized function. Stores the function itself
/// and `n` cells for memoization of different function args.
/// Do not construct directly, use memoize() function instead.
template <unsigned n, typename Func, typename Ret, typename... Args>
class Memoized {
public:
    explicit Memoized(Func func): func(func) {}

    Ret operator() (Args... args)
    {
        // On first call:
        // - field #0 is initialized to result of calling func with default-constructed args
        // - reading starts with this pre-initialized field #0
        // - if not matched, new item is written to field #1
        // On following calls:
        // - check recent items (backwards)
        // - write new items to uninitialized fields
        // - field #0 (default value) is overwritten last
        // - until all items are overwritten, lookup always goes through default in field #0

        int i = last_read_i;
        do {
            if (memo_args[i] == std::tuple<Args...>{args...})
                return memo_result[i];
            i = (i - 1 + n) % n;
        } while (i != last_read_i);

        i = (last_written_i + 1) % n;
        last_read_i = last_written_i = i;
        memo_args[i] = {args...};
        return memo_result[i] = func(args...);
    }

private:
    Func func;
    std::tuple<Args...> memo_args[n] {};
    Ret memo_result[n] {func(Args{}...)};
    int last_read_i = 0;
    int last_written_i = 0;
};


/// Make function object for memoization of `func`.
/// \tparam n       How many most-recent results should be remembered.
/// \param func     The function to be memoized and called by Memoized function object.
template<unsigned n = 8, typename Ret, typename... Args>
auto memoize(Ret (*func)(Args...)) {
    return Memoized<n, decltype(func), Ret, Args...>{func};
}


} // namespace xci::core

#endif // include guard
