// BinaryBase.h created on 2019-03-14, part of XCI toolkit
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

#ifndef XCI_DATA_BINARY_BASE_H
#define XCI_DATA_BINARY_BASE_H

#include <zlib.h>
#include <cstdint>

namespace xci::data {


class BinaryBase {
protected:
    enum {
        // --- HEADER ---
        // magic: CBDF (Chunked Binary Data Format)
        Magic_Byte0         = 0xCB,
        Magic_Byte1         = 0xDF,
        // version: 0 (first version of the format)
        Version             = 0,
        // flags
        Flags_LittleEndian  = 0b00000001, // bits 0,1 = endianness (values 00,11 - reserved)
        Flags_BigEndian     = 0b00000010,

        // --- CHUNK ---
        // type
        Type_Master         = 0b00000000, // VALUE is sequence of sub-chunks
        Type_String         = 0b00100000, // 3 bits, UTF-8 string (000 - reserved)
        Type_Integer        = 0b01000000, // any int/uint/enum/bool
        Type_Float          = 0b01100000, // (100,101,110 - reserved)
        Type_Checksum       = 0b11100000, // type of checksum depends on LEN
        Type_Mask           = 0b11100000,
        // length
        Length41_Mask       = 0b00001111,
        Length41_Flag       = 0b00010000,
        Length62_Mask       = 0b00111111,
        Length62_Flag0      = 0b00000000,
        Length62_Flag1      = 0b01000000,
        Length62_Flag2      = 0b10000000,
        Length62_Flag3      = 0b11000000,
        Length62_FlagMask   = 0b11000000,
        // master chunk
        Master_Enter        = 0b00000000,    // ORed with Type_Master and depth
        Master_Leave        = 0b00010000,    // ... enter/leave sub-chunks
    };

    uint32_t m_crc = 0;
};


} // namespace xci::data

#endif // include guard
