// Heap.cpp created on 2019-08-17 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Heap.h"

namespace xci::script {


HeapSlot::HeapSlot(size_t size)
    : m_slot(new std::byte[sizeof(uint32_t) + size])
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
    if (refs == 0) {
        delete[] m_slot;
    } else {
        memcpy(m_slot, &refs, sizeof(refs));
    }
}


uint32_t HeapSlot::refcount() const
{
    if (m_slot == nullptr)
        return 0;
    return bit_read<uint32_t>(m_slot);
}


} // namespace xci::script
