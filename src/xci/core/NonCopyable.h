// NonCopyable.h created on 2019-12-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_NONCOPYABLE_H
#define XCI_CORE_NONCOPYABLE_H

namespace xci::core {


class NonCopyable {
public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator =(const NonCopyable&) = delete;

protected:
    NonCopyable() = default;
};


class NonMovable {
public:
    NonMovable(NonCopyable&&) = delete;
    NonMovable& operator =(NonMovable&&) = delete;

protected:
    NonMovable() = default;
};


} // namespace xci::core

#endif // include guard
