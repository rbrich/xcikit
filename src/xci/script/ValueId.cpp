// ValueId.cpp created on 2026-11-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2026 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ValueId.h"
#include "Value.h"

#include <ranges>


namespace xci::script {


static thread_local std::deque<Value> tl_value_pool;


std::deque<Value>& ValueId::value_pool()
{
    return tl_value_pool;
}


ValueId ValueId::add(Value value)
{
    const auto it = std::ranges::find(value_pool(), value);
    if (it == value_pool().end()) {
        value.incref();
        value_pool().emplace_back(std::move(value));
        return ValueId(value_pool().size() - 1);
    }
    return it - value_pool().begin();
}

const Value& ValueId::get() const
{
    assert(m_id < value_pool().size());
    return value_pool()[m_id];
}


} // namespace xci::script
