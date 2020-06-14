// BinaryWriter.cpp created on 2019-03-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "BinaryWriter.h"
#include <xci/data/coding/leb128.h>

namespace xci::data {


void BinaryWriter::write_content()
{
    uint8_t flags = 0;

#if BYTE_ORDER == LITTLE_ENDIAN
    flags |=LittleEndian;
#endif
#if BYTE_ORDER == BIG_ENDIAN
    flags |= BigEndian;
#endif

    if (m_crc32)
        flags |= ChecksumCrc32;

    // Prepare header:
    // 4 bytes fixed header: MAGIC:16, VERSION:8, FLAGS:8
    // 6 bytes for SIZE in LEB128 => up to 4TB of file content
    uint8_t header[10] {
        Magic0, Magic1,
        Version,
        flags
    };

    uint8_t* iter = header + 4;
    assert(is_root_group());
    assert(group_buffer().size() < 0x400'0000'0000LLU);  // up to 4TB
    encode_leb128(iter, group_buffer().size());

    const size_t header_size = iter - header;
    assert(header_size <= 10);

    // Write header
    m_stream.write((const char*)header, header_size);

    // Write content
    m_stream.write((const char*)group_buffer().data(), group_buffer().size());

    if (!m_crc32)
        return;  // no checksum -> we're done

    // CRC-32: header + content
    Crc32 crc;
    crc.feed((const std::byte*)header, header_size);
    crc(group_buffer());

    // Write metadata intro (included in checksum)
    const uint8_t meta_intro = (Type::Control | 0);
    m_stream.put(meta_intro);
    crc(meta_intro);

    // Write checksum
    const uint8_t crc_intro = (Type::UInt32 | 1);
    m_stream.put(crc_intro);
    crc(crc_intro);
    m_stream.write((const char*)crc.data(), crc.size());
}


void BinaryWriter::enter_group(uint8_t key)
{
    m_group_stack.emplace_back();
}


void BinaryWriter::leave_group(uint8_t key)
{
    auto inner_buffer = std::move(group_buffer());
    m_group_stack.pop_back();
    // TYPE:4, KEY:4
    write(uint8_t(Type::Master | key));
    // LEN:32
    auto out_iter = buffer_inserter();
    encode_leb128<size_t, decltype(out_iter), std::byte>(out_iter, inner_buffer.size());
    // VALUE
    write(inner_buffer.data(), inner_buffer.size());
}


} // namespace xci::data
