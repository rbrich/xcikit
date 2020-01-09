// Heap.cpp created on 2019-08-17, part of XCI toolkit
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

#include "Heap.h"

namespace xci::script {


HeapSlot::HeapSlot(size_t size)
    : m_slot(new byte[sizeof(uint32_t) + size])
{
    uint32_t refs = 1;
    memcpy(m_slot, &refs, sizeof(refs));
}


void HeapSlot::incref() const
{
    if (m_slot == nullptr)
        return;
    const auto refs = bit_read<uint32_t>(m_slot) + 1;
    memcpy(m_slot, &refs, sizeof(refs));
}


void HeapSlot::decref() const
{
    if (m_slot == nullptr)
        return;
    const auto refs = bit_read<uint32_t>(m_slot) - 1;
    memcpy(m_slot, &refs, sizeof(refs));
}


uint32_t HeapSlot::refcount() const
{
    if (m_slot == nullptr)
        return 0;
    return bit_read<uint32_t>(m_slot);
}


void HeapSlot::gc()
{
    if (m_slot == nullptr)
        return;
    const auto refs = bit_read<uint32_t>(m_slot);
    if (refs == 0) {
        delete[] m_slot;
        m_slot = nullptr;
        return;
    }
}


} // namespace xci::script
