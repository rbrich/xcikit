// BinaryReader.cpp created on 2019-03-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "BinaryReader.h"
#include <zlib.h>

namespace xci::data {


void BinaryReader::read_header() {
    // This size is decreased with every read.
    // Initial value is used only for reading the header,
    // then overwritten by actual value from the header.
    group_buffer().size = 10;

    uint8_t header[4];
    read_with_crc((std::byte*)header, sizeof(header));

    // MAGIC:16
    if (header[0] != Magic0 || header[1] != Magic1)
        throw ArchiveBadMagic();

    // VERSION:8
    if (header[2] != Version)
        throw ArchiveBadVersion();

    // FLAGS:8
    auto endianness = (header[3] & EndiannessMask);
    if (endianness != LittleEndian)
        throw ArchiveBadFlags();
    m_flags = header[3];

    // add header to CRC checksum
    if (has_crc())
        m_crc(header);

    // SIZE:var
    group_buffer().size = read_leb128<size_t>();
}


void BinaryReader::read_footer()
{
    // read chunks until Control/Metadata
    for (;;) {
        if (group_buffer().size == 0) {
            if (has_crc())
                throw ArchiveMissingChecksum();
            return;  // no footer
        }
        uint8_t b;
        read_with_crc(b);
        const uint8_t chunk_key = b & KeyMask;
        const uint8_t chunk_type = b & TypeMask;
        if (chunk_type != Type::Control || chunk_key != Metadata) {
            skip_unknown_chunk(chunk_type, chunk_key);
            continue;
        }
        break;  // footer found
    }
    // read metadata chunks
    while (group_buffer().size != 0) {
        uint8_t b;
        read_with_crc(b);
        const uint8_t chunk_key = b & KeyMask;
        const uint8_t chunk_type = b & TypeMask;
        if (has_crc() && chunk_key == 1 && chunk_type == UInt32) {
            // stop feeding CRC
            m_flags = 0;
            // check CRC
            uint32_t stored_crc = 0;
            read_with_crc(stored_crc);
            if (stored_crc != m_crc.as_uint32())
                throw ArchiveBadChecksum();
            continue;
        }
        // unknown chunk
        skip_unknown_chunk(chunk_type, chunk_key);
    }
}


auto BinaryReader::generic_next() -> GenericNext
{
    if (group_buffer().size == 0) {
        // leave group
        if (is_root_group())
            return EndOfFile;
        m_group_stack.pop_back();
        return LeaveGroup;
    }

    auto b = (uint8_t) read_byte_with_crc();
    const uint8_t chunk_key = b & KeyMask;
    const uint8_t chunk_type = b & TypeMask;

    if (chunk_type == Type::Master) {
        // enter group
        auto chunk_length = read_leb128<size_t>();
        if (chunk_length > group_buffer().size)
            throw ArchiveUnexpectedEnd();
        // "move" the content from parent buffer to new child
        group_buffer().size -= chunk_length;
        m_group_stack.emplace_back();
        group_buffer().size = chunk_length;
        if (m_unknown_chunk_cb) {
            m_unknown_chunk_cb(chunk_type, chunk_key, nullptr, chunk_length);
        }
        return EnterGroup;
    }

    skip_unknown_chunk(chunk_type, chunk_key);
    return GenericItem;
}


void BinaryReader::enter_group(uint8_t key, const char* name)
{
    auto chunk_type = read_chunk_head(key);
    size_t chunk_length = 0;  // ChunkNotFound -> size 0
    if (chunk_type == Type::Master) {
        chunk_length = read_leb128<size_t>();
    } else if (chunk_type != ChunkNotFound)
        throw ArchiveBadChunkType();
    if (chunk_length > group_buffer().size)
        throw ArchiveUnexpectedEnd();
    // "move" the content from parent buffer to new child
    group_buffer().size -= chunk_length;
    m_group_stack.emplace_back();
    group_buffer().size = chunk_length;
}


void BinaryReader::leave_group(uint8_t key, const char* name)
{
    // drain the rest of chunks in the group
    auto chunk_type = read_chunk_head(ChunkNotFound);
    (void) chunk_type;
    assert(chunk_type == ChunkNotFound && group_buffer().size == 0);
    m_group_stack.pop_back();
}


void BinaryReader::skip_unknown_chunk(uint8_t type, uint8_t key)
{
    size_t length;
    if (type_has_len(type)) {
        length = read_leb128<size_t>();
    } else {
        length = size_by_type(type);
    }
    auto buf = std::make_unique<std::byte[]>(length);
    read_with_crc(buf.get(), length);
    if (m_unknown_chunk_cb) {
        m_unknown_chunk_cb(type, key, buf.get(), length);
    }
}


uint8_t BinaryReader::peek_chunk_head(uint8_t key)
{
    // Read ahead until key matches
    while (group_buffer().size != 0) {
        auto b = (uint8_t) peek_byte();
        const uint8_t chunk_key = b & KeyMask;
        const uint8_t chunk_type = b & TypeMask;
        if (chunk_key < key) {
            (void) read_byte_with_crc();
            skip_unknown_chunk(chunk_type, chunk_key);
            continue;
        }
        if (chunk_key > key)
            return ChunkNotFound;
        // success, pointer is still at KEY/TYPE
        return chunk_type;
    }
    return ChunkNotFound;
}


uint8_t BinaryReader::read_chunk_head(uint8_t key)
{
    auto chunk_type = peek_chunk_head(key);
    if (chunk_type == ChunkNotFound)
        return ChunkNotFound;
    read_byte_with_crc();
    return chunk_type;
}


void BinaryReader::read_with_crc(std::byte* buffer, size_t length)
{
    if (length > group_buffer().size)
        throw ArchiveUnexpectedEnd();
    group_buffer().size -= length;
    m_stream.read((char*)buffer, length);
    if (!m_stream)
        throw ArchiveReadError();
    if (has_crc())
        m_crc.feed(buffer, length);
}


std::byte BinaryReader::read_byte_with_crc()
{
    if (group_buffer().size-- == 0 || m_stream.eof())
        throw ArchiveUnexpectedEnd();
    char c = m_stream.get();
    if (!m_stream)
        throw ArchiveReadError();
    if (has_crc())
        m_crc(c);
    return (std::byte) c;
}


std::byte BinaryReader::peek_byte()
{
    if (m_stream.eof())
        throw ArchiveUnexpectedEnd();
    return (std::byte) m_stream.peek();
}


} // namespace xci::data
