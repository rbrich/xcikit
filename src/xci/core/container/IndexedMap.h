// IndexedMap.h created on 2020-03-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_INDEXED_MAP_H
#define XCI_CORE_INDEXED_MAP_H

#include <xci/core/mixin.h>
#include <vector>
#include <memory>
#include <new>
#include <bit>
#include <cassert>
#include <cstring>

namespace xci::core {


/// IndexedMap container
///
/// A specialized variant of std::deque. Composed of vector of sparse chunks.
/// Each chunk contains fixed number of slots and a bitmap of free slots.
///
/// Properties:
/// - stable references, stable Indexes
/// - each object has unique handle: WeakIndex
///     - auto-generated on insert
///     - contains Index + Tenant identifier
///     - Index points to a slot
///     - Tenant identifies the object in the slot, grows with every insert
///       into the same slot
///     - after deletion, the object's WeakIndex becomes invalid,
///       all other references and iterators stay valid
///  - iteration is almost as fast as std::deque
///
/// \tparam T   the element type

template < class T >
class IndexedMap: private NonCopyable {
public:

    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using difference_type = std::ptrdiff_t;
    using size_type = uint32_t;

    using Index = uint32_t;
    using Tenant = uint32_t;

    /// Persistent object handle.
    /// This isn't invalidated by any operation.
    /// Behaves like a weak pointer - after removing the element,
    /// the WeakIndex becomes invalid.
    /// When this safety is not needed, use just the Index part.
    struct WeakIndex {
        Index index = ~0u;
        Tenant tenant = ~0u;
        bool operator==(const WeakIndex& rhs) const noexcept = default;
    };

    static constexpr Index no_index {~0u};
    static constexpr WeakIndex not_found {};

private:

    struct Slot {
        T elem;
        Tenant tenant;  // generation number, 0 when allocated, increased on removal
    };

    struct Chunk {
        Slot* slot;
        uint64_t free_map;  // bitmap, 0=free, 1=occupied, mask: 1 << idx
        Index next_free;    // index of next chunk with free slots
    };
    static constexpr size_t chunk_size = 64;

public:

    IndexedMap() = default;
    IndexedMap(IndexedMap&& other) noexcept;
    ~IndexedMap() { destroy_chunks(); }

    IndexedMap& operator=(IndexedMap&& other) noexcept;

    bool operator==(const IndexedMap& other) const;
    bool operator!=(const IndexedMap& other) const { return !(*this == other); }

    /// Current capacity of underlying element storage.
    size_type capacity() const noexcept;

    /// Resize internal structures to fit occupied size.
    /// No iterators or references are invalidated.
    void shrink_to_fit() { m_chunk.shrink_to_fit(); }

    bool empty() const { return m_size == 0; }
    size_type size() const { return m_size; }
    void clear() noexcept;

    /// Create new object in first free slot.
    /// The returned slot ID might be reused (thus resurrecting a dead ID).
    template<class ...Args> WeakIndex emplace(Args&&... args);

    /// Add new object to first free slot.
    WeakIndex add(T&& value);
    WeakIndex add(const T& value) { return add(T{value}); }

    void remove(Index index);
    bool remove(WeakIndex weak_index);

    /// Get the object if still available, nullptr otherwise.
    T* get(WeakIndex weak);
    const T* get(WeakIndex weak) const;

    /// Get reference to object at index.
    /// Unsafe. Using wrong index may return garbage or crash.
    reference operator[](Index index);
    const_reference operator[](Index index) const;

    class iterator {
        friend IndexedMap;
    public:
        using difference_type = typename IndexedMap<T>::difference_type;
        using value_type = typename IndexedMap<T>::value_type;
        using reference = T&;
        using pointer = T*;
        using iterator_category = std::forward_iterator_tag;

        bool operator==(const iterator& rhs) const { return m_chunk_idx == rhs.m_chunk_idx && m_slot_idx == rhs.m_slot_idx; }
        bool operator!=(const iterator& rhs) const { return m_chunk_idx != rhs.m_chunk_idx || m_slot_idx != rhs.m_slot_idx; }

        iterator& operator++() {
            ++m_slot_idx;
            jump_to_next();
            return *this;
        }
        iterator operator++(int) { auto v = *this; ++*this; return v; }

        reference operator*() const { return (*m_chunk)[m_chunk_idx].slot[m_slot_idx].elem; }
        pointer operator->() const { return &(*m_chunk)[m_chunk_idx].slot[m_slot_idx].elem; }

        Index index() const { return m_chunk_idx * chunk_size + m_slot_idx; }
        Tenant tenant() const { return (*m_chunk)[m_chunk_idx].slot[m_slot_idx].tenant; }
        WeakIndex weak_index() const { return {index(), tenant()}; }

    private:
        explicit iterator(std::vector<Chunk>& chunk) : m_chunk(&chunk) {}
        explicit iterator(std::vector<Chunk>& chunk, Index) : m_chunk(&chunk), m_chunk_idx(0), m_slot_idx(0) {
            jump_to_next();
        }

        void jump_to_next();

        std::vector<Chunk>* m_chunk;
        Index m_chunk_idx = no_index;
        Index m_slot_idx = no_index;
    };

    class const_iterator {
        friend IndexedMap;
    public:
        using difference_type = typename IndexedMap<T>::difference_type;
        using value_type = typename IndexedMap<T>::value_type;
        using reference = const T&;
        using pointer = const T*;
        using iterator_category = std::forward_iterator_tag;

        bool operator==(const const_iterator& rhs) const { return m_chunk_idx == rhs.m_chunk_idx && m_slot_idx == rhs.m_slot_idx; }
        bool operator!=(const const_iterator& rhs) const { return m_chunk_idx != rhs.m_chunk_idx || m_slot_idx != rhs.m_slot_idx; }

        const_iterator& operator++() {
            ++m_slot_idx;
            jump_to_next();
            return *this;
        }
        const_iterator operator++(int) { auto v = *this; ++*this; return v; }

        reference operator*() const { return (*m_chunk)[m_chunk_idx].slot[m_slot_idx].elem; }
        pointer operator->() const { return &(*m_chunk)[m_chunk_idx].slot[m_slot_idx].elem; }

        Index index() const { return m_chunk_idx * chunk_size + m_slot_idx; }
        Tenant tenant() const { return (*m_chunk)[m_chunk_idx].slot[m_slot_idx].tenant; }
        WeakIndex weak_index() const { return {index(), tenant()}; }

    private:
        explicit const_iterator(const std::vector<Chunk>& chunk) : m_chunk(&chunk) {}
        explicit const_iterator(const std::vector<Chunk>& chunk, Index) : m_chunk(&chunk), m_chunk_idx(0), m_slot_idx(0) {
            jump_to_next();
        }

        void jump_to_next();

        const std::vector<Chunk>* m_chunk;
        Index m_chunk_idx = no_index;
        Index m_slot_idx = no_index;
    };

    iterator begin()                { return iterator{m_chunk, 0}; }
    const_iterator begin() const    { return const_iterator{m_chunk, 0}; }
    const_iterator cbegin() const   { return const_iterator{m_chunk, 0}; }
    iterator end()                  { return iterator{m_chunk}; }
    const_iterator end() const      { return const_iterator{m_chunk}; }
    const_iterator cend() const     { return const_iterator{m_chunk}; }

private:
    Slot* allocate_slots();
    void free_slots(Slot* slot);
    void destroy_chunks();  // deallocate all chunks

    Slot& acquire_slot(Index& index);

    std::vector<Chunk> m_chunk;
    size_type m_size = 0;
    Index m_free_chunk = no_index;  // index of first chunk with free slots
};


// ----------------------------------------------------------------------------
// Implementation details


template<class T>
IndexedMap<T>::IndexedMap(IndexedMap&& other) noexcept
    : m_chunk(std::move(other.m_chunk)),
      m_size(other.m_size),
      m_free_chunk(other.m_free_chunk)
{
    if (&other == this)
        return;
    assert(other.m_chunk.empty());
    other.m_size = 0;
    other.m_free_chunk = no_index;
}


template<class T>
IndexedMap<T>& IndexedMap<T>::operator=(IndexedMap&& other) noexcept
{
    m_chunk = std::move(other.m_chunk);
    m_size = other.m_size;
    m_free_chunk = other.m_free_chunk;
    other.m_size = 0;
    other.m_free_chunk = no_index;
    return *this;
}


template<class T>
bool IndexedMap<T>::operator==(const IndexedMap& other) const
{
    return size() == other.size() &&
           std::equal(cbegin(), cend(), other.cbegin());
}


template<class T>
auto IndexedMap<T>::capacity() const noexcept -> size_type
{
    return size_type(m_chunk.size() * chunk_size);
}


template<class T>
void IndexedMap<T>::clear() noexcept
{
    destroy_chunks();
    m_chunk.clear();
    m_size = 0;
    m_free_chunk = no_index;
}


template<class T>
template<class... Args>
auto IndexedMap<T>::emplace(Args&&... args) -> WeakIndex
{
    Index index;
    Slot& slot = acquire_slot(index);
    std::construct_at(&slot.elem, std::forward<Args>(args)...);
    return {index, slot.tenant};
}


template<class T>
auto IndexedMap<T>::add(T&& value) -> WeakIndex
{
    Index index;
    Slot& slot = acquire_slot(index);
    std::construct_at(&slot.elem, std::forward<T>(value));
    return {index, slot.tenant};
}


template<class T>
void IndexedMap<T>::remove(Index index)
{
    const size_t chunk_idx = index / chunk_size;
    assert(chunk_idx < m_chunk.size());
    Chunk& chunk = m_chunk[chunk_idx];

    const size_t slot_idx = index % chunk_size;
    assert((chunk.free_map & (uint64_t{1} << slot_idx)) == 1);  // slot is occupied
    if (chunk.free_map == 0) {
        // chunk was fully occupied, now freeing a slot
        chunk.next_free = m_free_chunk;
        m_free_chunk = chunk_idx;
    }
    chunk.free_map &= ~(uint64_t{1} << slot_idx);

    Slot& slot = chunk.slot[slot_idx];
    std::destroy_at(&slot.elem);
    ++ slot.tenant;
}


template<class T>
bool IndexedMap<T>::remove(WeakIndex weak_index)
{
    const Index chunk_idx = weak_index.index / chunk_size;
    const size_t slot_idx = weak_index.index % chunk_size;

    if (chunk_idx >= m_chunk.size())
        return false;  // index out of range

    Chunk& chunk = m_chunk[chunk_idx];
    if ((chunk.free_map & (uint64_t{1} << slot_idx)) == 0)
        return false;  // slot is not occupied
    if (chunk.slot[slot_idx].tenant != weak_index.tenant)
        return false;  // slot has different tenant

    if (chunk.free_map == ~uint64_t{0}) {
        // chunk was fully occupied, now freeing a slot
        chunk.next_free = m_free_chunk;
        m_free_chunk = chunk_idx;
    }
    chunk.free_map &= ~(uint64_t{1} << slot_idx);

    Slot& slot = chunk.slot[slot_idx];
    std::destroy_at(&slot.elem);
    ++ slot.tenant;
    return true;
}


template<class T>
T* IndexedMap<T>::get(IndexedMap::WeakIndex weak)
{
    return const_cast<T*>(const_cast<const IndexedMap<T>*>(this)->get(weak));
}


template<class T>
const T* IndexedMap<T>::get(IndexedMap::WeakIndex weak) const
{
    const size_t chunk_index = weak.index / chunk_size;
    if (chunk_index >= m_chunk.size())
        return nullptr;  // out of range
    const auto& chunk = m_chunk[chunk_index];
    const size_t slot_index = weak.index % chunk_size;
    if ((chunk.free_map & (uint64_t{1} << slot_index)) == 0)
        return nullptr;  // slot is free
    const auto& slot = chunk.slot[slot_index];
    if (slot.tenant != weak.tenant)
        return nullptr;
    return &slot.elem;
}


template<class T>
auto IndexedMap<T>::operator[](Index index) -> reference
{
    return m_chunk[index / chunk_size].slot[index % chunk_size].elem;
}


template<class T>
auto IndexedMap<T>::operator[](Index index) const -> const_reference
{
    return m_chunk[index / chunk_size].slot[index % chunk_size].elem;
}


template<class T>
auto IndexedMap<T>::allocate_slots() -> Slot*
{
    size_t size = chunk_size * sizeof(Slot);
    void* buffer = ::operator new(size, std::align_val_t(alignof(Slot)));
    std::memset(buffer, 0, size);
    return (Slot*) buffer;
}


template<class T>
void IndexedMap<T>::free_slots(Slot* slot)
{
    ::operator delete(slot, std::align_val_t(alignof(Slot)));
}


template<class T>
void IndexedMap<T>::destroy_chunks()
{
    for (Chunk chunk : m_chunk) {
        auto free_map = chunk.free_map;
        for (Slot* slot = chunk.slot; slot != chunk.slot + chunk_size; ++slot) {
            if (free_map & uint64_t{1})
                std::destroy_at(&slot->elem);
            free_map >>= 1;
            if (free_map == 0)
                break;
        }
        free_slots(chunk.slot);
    }
}


template<class T>
auto IndexedMap<T>::acquire_slot(Index& index) -> Slot&
{
    if (m_free_chunk == no_index) {
        m_free_chunk = Index(m_chunk.size());
        m_chunk.push_back({allocate_slots(), {}, no_index});
    }

    Chunk& chunk = m_chunk[m_free_chunk];
    auto free_slot = std::countr_one(chunk.free_map);
    chunk.free_map |= uint64_t{1} << free_slot;
    index = m_free_chunk * chunk_size + free_slot;
    if (chunk.free_map == ~uint64_t{0})
        m_free_chunk = chunk.next_free;
    ++m_size;
    return chunk.slot[free_slot];
}


template<class T>
void IndexedMap<T>::iterator::jump_to_next()
{
    if (m_slot_idx == chunk_size) {
        // end of chunk
        m_slot_idx = 0;
        ++m_chunk_idx;
    }
    while (m_chunk_idx < m_chunk->size()) {
        const Chunk& chunk = (*m_chunk)[m_chunk_idx];
        while ((chunk.free_map & (uint64_t{1} << m_slot_idx)) == 0) {
            // slot is free, jump to next one
            ++m_slot_idx;
            if (m_slot_idx == chunk_size) {
                // end of chunk
                m_slot_idx = 0;
                ++m_chunk_idx;
                goto continue_outer_loop;
            }
        }

        // slot is occupied, we're done
        return;

        continue_outer_loop:
        continue;
    }

    // end of container
    m_chunk_idx = no_index;
    m_slot_idx = no_index;
}


template<class T>
void IndexedMap<T>::const_iterator::jump_to_next()
{
    if (m_slot_idx == chunk_size) {
        // end of chunk
        m_slot_idx = 0;
        ++m_chunk_idx;
    }
    while (m_chunk_idx < m_chunk->size()) {
        const Chunk& chunk = (*m_chunk)[m_chunk_idx];
        while ((chunk.free_map & (uint64_t{1} << m_slot_idx)) == 0) {
            // slot is free, jump to next one
            ++m_slot_idx;
            if (m_slot_idx == chunk_size) {
                // end of chunk
                m_slot_idx = 0;
                ++m_chunk_idx;
                goto continue_outer_loop;
            }
        }

        // slot is occupied, we're done
        return;

        continue_outer_loop:
        continue;
    }

    // end of container
    m_chunk_idx = no_index;
    m_slot_idx = no_index;
}


} // namespace xci::core

#endif // include guard
