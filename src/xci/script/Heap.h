// Heap.h created on 2019-08-17 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_HEAP_H
#define XCI_SCRIPT_HEAP_H

#include <xci/core/bit.h>
#include <cstddef>  // byte
#include <cstdint>

namespace xci::script {


// Manually reference-counted heap slot
// Every instance on the stack should increase refcount by one.
// Single instance pulled of the stack retains one refcount, which needs to be
// manually decreased before destroying the object.
//
// The slot has a header followed by user data.
// Header is:
// * 4B refcount
// * zB pointer to deleter - a function to be called before destroying the slot
class HeapSlot {
public:
    using RefCount = uint32_t;
    using Deleter = void(*)(std::byte* data);
    static constexpr size_t header_size = sizeof(RefCount) + sizeof(Deleter);

    // new uninitialized slot
    HeapSlot() = default;
    // bind to existing slot
    explicit HeapSlot(std::byte* slot) : m_slot(slot) {}
    // create new slot with refcount = 1
    explicit HeapSlot(size_t user_size, Deleter deleter = nullptr);
    // copy existing slot
    HeapSlot(const HeapSlot& other) = default;
    HeapSlot(HeapSlot&& other) noexcept : m_slot(other.m_slot) { other.m_slot = nullptr; }
    HeapSlot& operator =(const HeapSlot& rhs) = default;
    HeapSlot& operator =(HeapSlot&& rhs) noexcept { std::swap(m_slot, rhs.m_slot); return *this; }

    void write(std::byte* buffer) const { std::memcpy(buffer, &m_slot, sizeof(m_slot)); }
    void read(const std::byte* buffer) { std::memcpy(&m_slot, buffer, sizeof(m_slot)); }

    RefCount refcount() const;
    void incref() const;  // constness is disputable here, but logically the object is not affected, only its refcount
    bool decref();  // free the object and return true when refcount = 0

    std::byte* data() { return data_(); }
    const std::byte* data() const { return data_(); }
    const std::byte* slot() const { return m_slot; }

    /// Release the object, ignoring refcount, not calling deleter.
    /// Use only after bit-copying the data to another HeapSlot.
    void release() { delete[] m_slot; m_slot = nullptr; }

    bool operator==(const HeapSlot&) const = default;
    explicit operator bool() const { return m_slot != nullptr; }

private:
    std::byte* data_() const { return m_slot == nullptr ? nullptr : m_slot + header_size; }

    std::byte* m_slot = nullptr;
};


} // namespace xci::script

#endif // include guard
