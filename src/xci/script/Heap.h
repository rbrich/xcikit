// Heap.h created on 2019-08-17, part of XCI toolkit
// Copyright 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef XCI_SCRIPT_HEAP_H
#define XCI_SCRIPT_HEAP_H

#include <xci/compat/bit.h>
#include <cstddef>  // byte
#include <cstdint>

namespace xci::script {


// Manually reference-counted heap slot
// Every instance on the stack should increase refcount by one.
// Single instance pulled of the stack retains one refcount, which needs to be
// manually decreased before destroying the object.
class HeapSlot {
public:
    // new uninitialized slot
    HeapSlot() = default;
    // bind to existing slot
    explicit HeapSlot(std::byte* slot) : m_slot(slot) {}
    // create new slot with refcount = 1
    explicit HeapSlot(size_t size);
    // free the object only when refcount = 0
    ~HeapSlot() { gc(); }
    // copy existing slot
    HeapSlot(const HeapSlot& other) = default;
    HeapSlot(HeapSlot&& other) noexcept : m_slot(other.m_slot) { other.m_slot = nullptr; }
    HeapSlot& operator =(const HeapSlot& rhs) = default;
    HeapSlot& operator =(HeapSlot&& rhs) noexcept { std::swap(m_slot, rhs.m_slot); return *this; }

    void write(std::byte* buffer) const { std::memcpy(buffer, &m_slot, sizeof(m_slot)); }
    void read(const std::byte* buffer) { std::memcpy(&m_slot, buffer, sizeof(m_slot)); }

    void incref() const;
    void decref() const;
    uint32_t refcount() const;
    void gc();  // delete slot if refs=0

    std::byte* data() { return m_slot == nullptr ? nullptr : m_slot + 4; }
    const std::byte* data() const { return m_slot == nullptr ? nullptr : m_slot + 4; }
    const std::byte* slot() const { return m_slot; }

    explicit operator bool() const { return m_slot != nullptr; }

private:
    std::byte* m_slot = nullptr;
};


} // namespace xci::script

#endif // include guard
