// BinaryWriter.cpp created on 2019-03-13, part of XCI toolkit
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

#include "BinaryWriter.h"
#include <vector>

namespace xci::data {


void BinaryWriter::write_header()
{
    // initialize CRC
    m_crc = (uint32_t) crc32(0L, Z_NULL, 0);
    m_pos = 0;

    // MAGIC:16
    uint8_t magic[2] = {Magic_Byte0, Magic_Byte1};
    write_with_crc(magic);

    // VERSION:8
    uint8_t version = Version;
    write_with_crc(version);

    // FLAGS:8
    uint8_t flags = 0;
#if BYTE_ORDER == LITTLE_ENDIAN
    flags |= Flags_LittleEndian;
#endif
#if BYTE_ORDER == BIG_ENDIAN
    flags |= Flags_BigEndian
#endif
    write_with_crc(flags);
}


void BinaryWriter::write_footer()
{
    // TAG + CRC:32
    write_type_len(Type_Checksum, sizeof(m_crc));
    m_stream.write((char*)&m_crc, sizeof(m_crc));
}


void BinaryWriter::write(const char* name, const std::string& value)
{
    // TYPE:3 LENFLAG:1 LEN:4 [LEN:8-64]
    write_type_len(Type_String, value.size());

    // KEY:8..24
    write_key(name);

    // VALUE
    write_with_crc((const uint8_t*)value.data(), value.size());
}


void BinaryWriter::write(const char* name, unsigned int value)
{
    write_type_len(Type_Integer, sizeof(value));
    write_key(name);
    write_with_crc(value);
}


void BinaryWriter::write(const char* name, double value)
{
    write_type_len(Type_Float, sizeof(value));
    write_key(name);
    write_with_crc(value);
}


void BinaryWriter::write_type_len(uint8_t type, uint64_t len)
{
    // TYPE:3 FLAG:1 LEN:4
    {
        uint8_t type_len = (type & Type_Mask) | uint8_t(len & Length41_Mask);
        len >>= 4;
        if (len > 0) {
            type_len |= Length41_Flag;
        }
        write_with_crc(type_len);
    }
    if (len == 0)
        return;

    // There might be 60 more bits of len. Those are encoded in 8,16,32 or 64 bits:
    //
    // FLAG:2 LEN:6 [LEN:8] [LEN:16] [LEN:32]
    //        \ F=0  \ F=1   \ F=2    \ F=3
    //
    // The first two bits tell how many more bits are used.

    // FLAG:2 LEN:6
    {
        auto len0 = uint8_t(len & Length62_Mask);
        len >>= 6;
        if (len == 0) {
            len0 |= Length62_Flag0;
        } else if (len < (1 << 8)) {
            len0 |= Length62_Flag1;
        } else if (len < (1 << 24)) {
            len0 |= Length62_Flag2;
        } else {
            len0 |= Length62_Flag3;
        }
        write_with_crc(len0);
    }
    if (len == 0)
        return;

    // LEN:8
    {
        auto len1 = uint8_t(len & 0xff);
        write_with_crc(len1);
        len >>= 8;
    }
    if (len == 0)
        return;

    // LEN:16
    {
        uint16_t len2 = htole16((uint16_t)(len & 0xffff));
        write_with_crc(len2);
        len >>= 16;
    }
    if (len == 0)
        return;

    // LEN:32
    {
        uint32_t len3 = htole32((uint32_t)len);
        write_with_crc(len3);
    }
}


void BinaryWriter::write_key(const char* key)
{
    // Try emplace
    auto item = m_key_to_pos.emplace(key, m_pos);
    // Is it new?
    if (item.second || m_pos - item.first->second > 0x7fff) {
        // Write raw key
        if (*key & 0x80) {
            // Prepend 0x02 to avoid the FLAG bit
            uint8_t placeholder = 0x02;
            write_with_crc(placeholder);
        }
        write_with_crc((const uint8_t*)key, strlen(key)+1);
    } else {
        // Write offset to first appearance
        auto offset = uint16_t(0x8000 | (m_pos - item.first->second));
        write_with_crc(offset);
    }
}


void BinaryWriter::write_master(int flag)
{
    if (flag == Master_Leave)
        m_depth--;

    auto type_len = uint8_t(Type_Master | flag | (m_depth & Length41_Mask));
    write_with_crc(type_len);

    if (flag == Master_Enter)
        m_depth++;
}


} // namespace xci::data
