// StringPool.h created on 2023-09-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

// References:
// - https://en.wikipedia.org/wiki/String_interning

#ifndef XCI_CORE_STRING_POOL_H
#define XCI_CORE_STRING_POOL_H

#include <vector>
#include <bit>
#include <cstdint>
#include <cassert>

namespace xci::core {


/// Pool of interned strings
///
/// Adding the same string twice will reliably return same ID.
/// The IDs can be used instead of comparing actual strings.
class StringPool {
public:
    StringPool(size_t initial_table_size = 64) {
        assert(std::has_single_bit(initial_table_size));  // The hash table size must be power of 2
        m_hash_table.resize(initial_table_size);
    }

    // The scheme of string ID:
    // * 0 = empty string
    // * rightmost bit = embed or pool:
    //   - 0 = 0..3 chars, no pool, the chars are stored directly in leftmost bytes (0-padded, 0-terminated)
    //   - 1 = string stored in `strings`, the ID minus 1 is byte offset (i.e. it's always aligned to 2 byte offset),
    //         the string is zero-terminated
    using Id = uint32_t;
    static constexpr Id empty_string = 0u;

    Id add(const char* str);

    // Id taken by ref so the result can point into it
    const char* get(const Id& id) const;

    size_t occupancy() const { return m_occupied; }

private:
    void grow_hash_table();

    static constexpr Id free_slot = 0u;
    struct Slot {
        uint32_t hash = 0;
        Id id = free_slot;  // (id - 1) = offset into m_strings
    };
    std::vector<Slot> m_hash_table;  // hash % size => Slot
    std::vector<char> m_strings;  // pooled zero-terminated strings
    size_t m_occupied = 0;
};


} // namespace xci::core

#endif  // include guard
