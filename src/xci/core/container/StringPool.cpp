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
        case 3: k ^= key[2] << 16; [[fallthrough]];
        case 2: k ^= key[1] << 8;  [[fallthrough]];
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

static constexpr uint32_t pool_mask = 0x8000'0000;  // the pooling bit (0=embed, 1=pool)
static constexpr uint32_t offset_mask = 0x7fff'ffff;

static XCI_INLINE StringPool::Id offset(StringPool::Id id) {
    return id & offset_mask;
}

auto StringPool::add(std::string_view str) -> Id
{
    if (str.size() <= 4) {
        // Small string optimization - up to 4 chars long
        Id res = 0;
        std::memcpy(&res, str.data(), str.size());
        // little endian - the last char in 4-char string must be in 7-bit range
        // big endian - the first char must be in 7-bit range
        if ((res & pool_mask) == 0)
            return res;
    }

    const auto hash = murmur3_32((const uint8_t*)str.data(), str.size());
    auto slot_i = hash % m_hash_table.size();
    for (;;) {
        const Slot& slot = m_hash_table[slot_i];
        if (slot.id == free_slot)
            break;
        if (slot.hash == hash) {
            // The existing string might be the same, compare it
            assert(m_strings.size() > offset(slot.id));
            const char* existing = m_strings.data() + offset(slot.id);
            if (str == existing)
                return slot.id;
        }
        slot_i = (slot_i + 1) % m_hash_table.size();
    }

    Id id = Id(m_strings.size()) | pool_mask;
    m_strings.insert(m_strings.end(), str.begin(), str.end());
    m_strings.push_back(0);
    m_hash_table[slot_i] = {hash, id};
    if (++m_occupied > 0.7 * m_hash_table.size())
        grow_hash_table();
    return id;
}


std::string_view StringPool::view(Id id) const
{
    if (id & pool_mask) {
        assert(m_strings.size() > offset(id));
        return m_strings.data() + offset(id);
    }
    static thread_local char str[4];
    std::memcpy(str, &id, 4);
    if (str[3] == 0)
        return str;
    else
        return {str, 4};
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
