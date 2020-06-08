// BinaryWriter.cpp created on 2019-03-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "BinaryWriter.h"

namespace xci::data {


void BinaryWriter::write_header()
{
    // reset state
    reset();
    m_terminated = false;

    // MAGIC:16
    uint8_t magic[2] = {Magic_Byte0, Magic_Byte1};
    write(magic);

    // VERSION:8
    uint8_t version = Version;
    write(version);

    // FLAGS:8
    uint8_t flags = 0;
#if BYTE_ORDER == LITTLE_ENDIAN
    flags |= Flags_LittleEndian;
#endif
#if BYTE_ORDER == BIG_ENDIAN
    flags |= Flags_BigEndian;
#endif
    write(flags);
}


void BinaryWriter::terminate_content()
{
    if (m_terminated)
        return;
    // TERMINATOR:8
    write(uint8_t(Type::Terminator | 0xf));
    assert(is_root_group());
}


void BinaryWriter::write_crc32(uint32_t crc)
{
    // CRC:32
    write(uint8_t(Type::UInt32 | 1));
    write(crc);
}


void BinaryWriter::write(const std::byte* buffer, size_t length)
{
    m_stream.write((const char*)buffer, length);
}


off_t BinaryWriter::enter(uint8_t key)
{
    write(uint8_t(Type::Master | key));
    write(uint32_t{});  // LEN:32
    return m_stream.tellp();
}


void BinaryWriter::leave(uint8_t key, off_t begin)
{
    // seek back and write length of the content
    auto end = m_stream.tellp();
    auto length = end - begin;
    m_stream.seekp(begin - 4);
    write(uint32_t(length));  // LEN:32
    // write terminator
    m_stream.seekp(end);
    write(uint8_t(Type::Terminator | key));
}


} // namespace xci::data
