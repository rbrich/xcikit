// Heap.cpp created on 2019-08-17 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Heap.h"

namespace xci::script {


HeapSlot::HeapSlot(size_t user_size, Deleter deleter)
    : m_slot(new std::byte[header_size + user_size])
{
    RefCount refs = 1;
    memcpy(m_slot, &refs, sizeof(refs));
    memcpy(m_slot + sizeof(RefCount), &deleter, sizeof(Deleter));
}


void HeapSlot::incref() const
{
    if (m_slot == nullptr)
        return;
    const auto refs = bit_copy<RefCount>(m_slot) + 1;
    memcpy(m_slot, &refs, sizeof(refs));
}


bool HeapSlot::decref() const
{
    if (m_slot == nullptr)
        return false;  // caller's pointer is already null
    const auto refs = bit_copy<RefCount>(m_slot) - 1;
    if (refs == 0) {
        Deleter deleter;
        memcpy(&deleter, m_slot + sizeof(RefCount), sizeof(Deleter));
        if (deleter != nullptr)
            deleter(data_());
        delete[] m_slot;
        return true;  // freed, the caller may want to clear the pointer
    } else {
        memcpy(m_slot, &refs, sizeof(refs));
        return false;
    }
}


auto HeapSlot::refcount() const -> RefCount
{
    if (m_slot == nullptr)
        return 0;
    return bit_copy<RefCount>(m_slot);
}


} // namespace xci::script
