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
#define FAIL_IF(cond,err) \
    if (cond) { m_stream.setstate(std::ios::failbit); m_error=err; return; }


void BinaryReader::read_header() {
    // initialize CRC
    m_crc = (uint32_t) crc32(0L, Z_NULL, 0);
    m_pos = 0;
    m_depth = 0;

    // MAGIC:16
    uint8_t magic[2];
    read_with_crc(magic);
    FAIL_IF(magic[0] != Magic_Byte0 || magic[1] != Magic_Byte1, Error::BadMagic);

    // VERSION:8
    uint8_t version = 0;
    read_with_crc(version);
    FAIL_IF(version != Version, Error::BadVersion);

    // FLAGS:8
    uint8_t flags = 0;
    read_with_crc(flags);
    FAIL_IF(flags == 0, Error::BadFlags);
}


void BinaryReader::read_footer()
{
    uint32_t crc = 0;
    m_stream.read((char*)&crc, sizeof(crc));
    FAIL_IF(crc != m_crc, Error::BadChecksum)
}


void BinaryReader::read_with_crc(uint8_t* buffer, size_t length)
{
    m_stream.read((char*)buffer, length);
    m_crc = (uint32_t) crc32(m_crc, buffer, (uInt)length);
    m_pos += length;
}


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


const char* BinaryReader::read_key()
{
    auto startpos = m_pos;
    uint8_t len = 0;
    read_with_crc(len);
    if (len & 0x80) {
        // Read offset, lookup prev key
        uint8_t len1 = 0;
        read_with_crc(len1);
        auto offset = ((len1 << 7) | (len & 0x7f));
        auto key_pos = startpos - offset;
        return m_pos_to_key[key_pos].c_str();
    }
    // Read key
    std::string key(len, 0);
    read_with_crc((uint8_t*)&key[0], len);
    // Save key
    auto& slot = m_pos_to_key[startpos];
    slot = std::move(key);
    return slot.c_str();
}


void BinaryReader::read(std::string& value)
{
    read_with_crc((uint8_t*)&value[0], value.size());
}


} // namespace xci::data
