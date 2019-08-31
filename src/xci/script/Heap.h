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
#include <xci/compat/utility.h>
#include <cstdint>

namespace xci::script {


// Reference counted heap slot
// Each instance holds 1 refcount
// Every push to stack increases refcount, pop from stack decreases refcount.
class HeapSlot {
public:
    // new uninitialized slot
    HeapSlot() : m_slot(nullptr) {}
    // initialized slot, just this one ref
    explicit HeapSlot(size_t size);
    // use existing slot, +1 ref
    explicit HeapSlot(byte* slot) : m_slot(slot) { incref(); }
    // copy existing slot
    HeapSlot(const HeapSlot& other) : m_slot(other.m_slot) { incref(); }
    HeapSlot(HeapSlot&& other) noexcept : m_slot(other.m_slot) { other.m_slot = nullptr; }
    HeapSlot& operator =(const HeapSlot& rhs) { reset(rhs.m_slot); return *this; }
    HeapSlot& operator =(HeapSlot&& rhs) noexcept { std::swap(m_slot, rhs.m_slot); return *this; }
    // delete instance, -1 refcount
    ~HeapSlot() { decref_gc(); }

    void write(byte* buffer) const { std::memcpy(buffer, &m_slot, sizeof(m_slot)); }
    void read(const byte* buffer) { std::memcpy(&m_slot, buffer, sizeof(m_slot)); }

    void incref() const;
    void decref() const;

    // decref + delete slot if refs=0
    void decref_gc();

    byte* data() { return m_slot == nullptr ? nullptr : m_slot + 4; }
    const byte* data() const { return m_slot == nullptr ? nullptr : m_slot + 4; }
    const byte* slot() const { return m_slot; }

    void reset(byte* slot);
    void reset(size_t size);

    explicit operator bool() const { return m_slot != nullptr; }

private:
    byte* m_slot;
};


} // namespace xci::script

#endif // include guard
