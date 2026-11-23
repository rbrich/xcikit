// ValueId.h created on 2026-11-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2026 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_VALUE_ID_H
#define XCI_SCRIPT_VALUE_ID_H

#include "SymbolTable.h"  // for Index, no_index
#include <deque>

namespace xci::script {

class Value;

// Strong type pointing into value_pool (container of Value)
// Used mainly for struct type default values
class ValueId {
public:
    ValueId() = default;
    ValueId(Index idx) : m_id(idx) {}

    // Thread-local pool of values (mainly default values in TypeInfo)
    // Note that the compiler is single-thread, so each compiler has its own instance of the pool.
    static std::deque<Value>& value_pool();

    // This is costly operation, keep it explicit - don't add it to a constructor
    static ValueId add(Value value);

    friend bool operator==(ValueId a, ValueId b) { return a.m_id == b.m_id; }
    friend bool operator<(ValueId a, ValueId b) { return a.m_id < b.m_id; }

    [[nodiscard]] Index id() const { return m_id; }
    [[nodiscard]] bool empty() const { return m_id == no_index; }
    explicit operator bool() const { return !empty(); }

    [[nodiscard]] const Value& get() const;

private:
    Index m_id = no_index;
};


} // namespace xci::script

#endif  // include guard
