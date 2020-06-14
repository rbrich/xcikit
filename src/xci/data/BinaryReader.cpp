// BinaryReader.cpp created on 2019-03-14, part of XCI toolkit
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

#include "BinaryReader.h"
#include <zlib.h>

namespace xci::data {

#define CHECK_FAIL      if (m_stream.fail()) return;
#define FAIL_IF(cond, err) \
    do { if (cond) { m_stream.setstate(std::ios::failbit); throw err; } } while(0)


void BinaryReader::read_header() {
    // This size is decreased with every read.
    // Initial value is used only for reading the header,
    // then overwritten by actual value from the header.
    group_buffer().size = 10;

    uint8_t header[4];
    read_with_crc((std::byte*)header, sizeof(header));

    // MAGIC:16
    FAIL_IF(header[0] != Magic0 || header[1] != Magic1, ArchiveBadMagic());

    // VERSION:8
    FAIL_IF(header[2] != Version, ArchiveBadVersion());

    // FLAGS:8
    auto endianness = (header[3] & EndiannessMask);
    FAIL_IF(endianness != LittleEndian, ArchiveBadFlags());
    m_has_crc = (header[3] & ChecksumMask) == ChecksumCrc32;

    // add header to CRC checksum
    if (m_has_crc)
        m_crc(header);

    // SIZE:var
    group_buffer().size = read_leb128<size_t>();
}


void BinaryReader::read_footer()
{
    // read chunks until Control/Metadata
    for (;;) {
        if (group_buffer().size == 0) {
            if (m_has_crc)
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
        if (m_has_crc && chunk_key == 1 && chunk_type == UInt32) {
            // stop feeding CRC
            m_has_crc = false;
            // check CRC
            uint32_t stored_crc = 0;
            read_with_crc(stored_crc);
            FAIL_IF(stored_crc != m_crc.as_uint32(), ArchiveBadChecksum());
            continue;
        }
        // unknown chunk
        skip_unknown_chunk(chunk_type, chunk_key);
    }
}


void BinaryReader::read_with_crc(std::byte* buffer, size_t length)
{
    if (length > group_buffer().size)
        throw ArchiveUnexpectedEnd();
    group_buffer().size -= length;
    m_stream.read((char*)buffer, length);
    if (m_has_crc)
        m_crc.feed(buffer, length);
}


std::byte BinaryReader::read_byte_with_crc()
{
    if (m_group_stack.back().buffer.size-- == 0)
        throw ArchiveUnexpectedEnd();
    char c = m_stream.get();
    if (m_has_crc)
        m_crc(c);
    return (std::byte) c;
}


std::byte BinaryReader::peek_byte()
{
    if (m_stream.eof())
        throw ArchiveUnexpectedEnd();
    return (std::byte) m_stream.peek();
}


/*
void BinaryReader::read_type_len(uint8_t& type, uint64_t& len)
{
    // TYPE:3 FLAG:1 LEN:4
    {
        uint8_t type_len = 0;
        read_with_crc(type_len);
        CHECK_FAIL;

        type = type_len & Type_Mask;
        len = type_len & Length41_Mask;
        bool flag = type_len & Length41_Flag;
        if (!flag)
            return;
    }

    // Read extended LEN (FLAG is set)
    //
    // FLAG:2 LEN:6 [LEN:8] [LEN:16] [LEN:32]
    //        \ F=0  \ F=1   \ F=2    \ F=3

    // FLAG:2 LEN:6
    uint8_t flag2;
    {
        uint8_t len0 = 0;
        read_with_crc(len0);
        len |= (len0 & Length62_Mask) << 4;
        flag2 = len0 & Length62_FlagMask;
    }
    if (flag2 == Length62_Flag0)
        return;

    // LEN:8
    {
        uint8_t len1 = 0;
        read_with_crc(len1);
        len |= len1 << 10;
    }
    if (flag2 == Length62_Flag1)
        return;

    // LEN:16
    {
        uint16_t len2 = 0;
        read_with_crc(len2);
        len |= le16toh(len2) << 18;
    }
    if (len == 0)
        return;

    // LEN:32
    {
        uint32_t len3 = 0;
        read_with_crc(len3);
        len |= uint64_t(le32toh(len3)) << 34;
    }
}
*/


void BinaryReader::read(std::string& value)
{
    read_with_crc((std::byte*)&value[0], value.size());
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


void BinaryReader::enter_group(uint8_t key)
{
    auto chunk_type = read_chunk_head(key);
    size_t chunk_length = 0;  // ChunkNotFound -> size 0
    if (chunk_type == Type::Master) {
        chunk_length = read_leb128<size_t>();
    } else if (chunk_type != ChunkNotFound)
        throw ArchiveBadChunkType();
    if (chunk_length > m_group_stack.size())
        throw ArchiveUnexpectedEnd();
    group_buffer().size -= chunk_type;
    m_group_stack.emplace_back();
    group_buffer().size = chunk_length;
}


void BinaryReader::leave_group(uint8_t key)
{
    auto chunk_type = read_chunk_head(ChunkNotFound);
    (void) chunk_type;
    assert(chunk_type == ChunkNotFound && group_buffer().size == 0);
    m_group_stack.pop_back();
}


} // namespace xci::data
