// mixin.h created on 2019-12-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_NONCOPYABLE_H
#define XCI_CORE_NONCOPYABLE_H

namespace xci::core {

/// Mixin classes. Use them as private base.
/// https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Non-copyable_Mixin

class NonCopyable {
public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator =(const NonCopyable&) = delete;

protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
    NonCopyable(NonCopyable&&) = default;
    NonCopyable& operator =(NonCopyable&&) = default;
};


class NonMovable {
public:
    NonMovable(NonMovable&&) = delete;
    NonMovable& operator =(NonMovable&&) = delete;

protected:
    NonMovable() = default;
    ~NonMovable() = default;
};


} // namespace xci::core

#endif // include guard
