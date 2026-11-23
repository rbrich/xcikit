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

void ValueId::clear_pool()
{
    for (auto& val : tl_value_pool) {
        val.decref();
    }
    tl_value_pool.clear();
}


ValueId ValueId::add(Value&& value)
{
    const auto it = std::ranges::find(value_pool(), value);
    if (it != value_pool().end()) {
        value.decref();  // we don't save the new value -> release it (note that newly created values have refcount=1)
        return ValueId(it - value_pool().begin());
    }
    value_pool().emplace_back(std::move(value));
    return ValueId(value_pool().size() - 1);
}

const Value& ValueId::get() const
{
    assert(m_id < value_pool().size());
    return value_pool()[m_id];
}


} // namespace xci::script
