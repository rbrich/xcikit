// Crc32.cpp created on 2020-06-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Crc32.h"
#include <zlib.h>

namespace xci::data {


inline uint32_t init_crc()
{
    return (uint32_t) crc32_z(0L, Z_NULL, 0);
}


Crc32::Crc32()
    : m_crc(init_crc())
{}


void Crc32::reset()
{
    m_crc = init_crc();
}


void Crc32::feed(const std::byte* data, size_t size)
{
    m_crc = (uint32_t) crc32_z(m_crc, (const Bytef*) data, size);
}


} // namespace xci::data
