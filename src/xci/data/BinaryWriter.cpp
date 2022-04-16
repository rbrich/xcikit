// BinaryWriter.cpp created on 2019-03-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "BinaryWriter.h"

namespace xci::data {


void BinaryWriter::write_content()
{
    uint8_t flags = 0;

#if BYTE_ORDER == LITTLE_ENDIAN
    flags |= LittleEndian;
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
    size_t content_size = group_buffer().size() + (m_crc32 ? 6 : 0);
    assert(uint64_t(content_size) < 0x400'0000'0000LLU);  // up to 4TB
    leb128_encode(iter, content_size);

    const ssize_t header_size = iter - header;
    assert(header_size <= 10);

    // Write header
    m_stream.write((const char*)header, header_size);

    // Write content
    m_stream.write((const char*)group_buffer().data(), std::streamsize(group_buffer().size()));

    if (!m_crc32)
        return;  // no checksum -> we're done

    // CRC-32: header + content
    Crc32 crc;
    crc.feed((const std::byte*)header, size_t(header_size));
    crc(group_buffer());

    // Write metadata intro (included in checksum)
    const uint8_t meta_intro = (Type::Control | 0);
    m_stream.put(char(meta_intro));
    crc(meta_intro);

    // Write checksum
    const uint8_t crc_intro = (Type::UInt32 | 1);
    m_stream.put(crc_intro);
    crc(crc_intro);
    m_stream.write((const char*)crc.data(), crc.size());
}


void BinaryWriter::write_group(uint8_t key, const char* name)
{
    if (key > 15)
        throw ArchiveOutOfKeys(key);

    auto inner_buffer = std::move(group_buffer());
    m_group_stack.pop_back();
    // TYPE:4, KEY:4
    write(uint8_t(Type::Master | key));
    // LEN:32
    write_leb128(inner_buffer.size());
    // VALUE
    write(inner_buffer.data(), inner_buffer.size());
}


} // namespace xci::data
