// StringPool.cpp created on 2023-09-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "StringPool.h"
#include <xci/compat/macros.h>
#include <cstring>

namespace xci::core {


// -----------------------------------------------------------------------------
// https://en.wikipedia.org/wiki/MurmurHash
// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp

static XCI_INLINE uint32_t murmur_32_scramble(uint32_t k) {
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}

static XCI_INLINE uint32_t murmur3_32(const uint8_t* key, size_t len)
{
    uint32_t h = 0u;  // seed
    uint32_t k;
    // Read in groups of 4
    for (size_t i = len >> 2; i; i--) {
        memcpy(&k, key, sizeof(uint32_t));
        key += sizeof(uint32_t);
        h ^= murmur_32_scramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }
    // Read the rest
    k = 0;
    switch (len & 3) {
        case 3: k ^= key[2] << 16;
        case 2: k ^= key[1] << 8;
        case 1: k ^= key[0];
    }
    h ^= murmur_32_scramble(k);
    // Finalize
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

// -----------------------------------------------------------------------------


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

    size = strlen(str);
    const auto hash = murmur3_32((const uint8_t*)str, size);
    auto slot_i = hash % m_hash_table.size();
    for (;;) {
        const Slot& slot = m_hash_table[slot_i];
        if (slot.id == free_slot)
            break;
        if (slot.hash == hash) {
            // The existing string might be the same, compare it
            assert(m_strings.size() > slot.id - 1);
            const char* existing = m_strings.data() + slot.id - 1;
            if (strcmp(str, existing) == 0)
                return slot.id;
        }
        slot_i = (slot_i + 1) % m_hash_table.size();
    }

    if (m_strings.size() & 1)
        m_strings.push_back(0);
    Id id = m_strings.size() + 1;
    m_strings.insert(m_strings.end(), str, str + size + 1);
    m_hash_table[slot_i] = {hash, id};
    if (++m_occupied > 0.7 * m_hash_table.size())
        grow_hash_table();
    return id;
}


const char* StringPool::get(const Id& id) const
{
    if (id & 1) {
        assert(m_strings.size() > id - 1);
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
