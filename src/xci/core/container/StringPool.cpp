// StringPool.cpp created on 2023-09-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "StringPool.h"
#include <cstring>

namespace xci::core {


auto StringPool::hash(const char* str) -> uint32_t
{
    return (uint32_t) std::hash<std::string_view>{}(str);
}


auto StringPool::add(const char* str) -> Id
{
    unsigned size = -1u;
    for (unsigned i = 0; i != 4; ++i) {
        if (str[i] == 0)
            size = i;
    }
    if (size < 4) {
        Id res = 0;
        std::memcpy(&res, str, size);
        return res;
    }

    const auto h = hash(str);
    auto slot_i = h % m_hash_table.size();
    for (;;) {
        const Slot& slot = m_hash_table[slot_i];
        if (slot.id == free_slot)
            break;
        if (slot.hash == h) {
            // The existing string might be the same, compare it
            assert(strings.size() > slot.id - 1);
            const char* existing = m_strings.data() + slot.id - 1;
            if (strcmp(str, existing) == 0)
                return slot.id;
        }
        slot_i = (slot_i + 1) % m_hash_table.size();
    }

    if (m_strings.size() & 1)
        m_strings.push_back(0);
    Id id = m_strings.size() + 1;
    m_strings.insert(m_strings.end(), str, str + strlen(str) + 1);
    m_hash_table[slot_i] = {h, id};
    if (++m_occupied > 0.7 * m_hash_table.size())
        grow_hash_table();
    return id;
}


const char* StringPool::get(const Id& id) const
{
    if (id & 1) {
        assert(strings.size() > id - 1);
        return m_strings.data() + id - 1;
    }
    return (const char*)&id;
}


void StringPool::grow_hash_table()
{
    auto old_table = std::move(m_hash_table);
    const auto new_size = old_table.size() * 2;
    m_hash_table.clear();
    m_hash_table.resize(new_size);
    for (auto& old_slot : old_table) {
        auto slot_i = old_slot.hash % new_size;
        for (;;) {
            Slot& slot = m_hash_table[slot_i];
            if (slot.id == free_slot) {
                slot = old_slot;
                break;
            }
            slot_i = (slot_i + 1) % new_size;
        }
    }
}


} // namespace xci::core
