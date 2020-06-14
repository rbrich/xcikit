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

#include <xci/core/error.h>
#include "Crc32.h"
#include <cstdint>
#include <vector>
#include <bitset>

namespace xci::data {


#ifdef __cpp_concepts

template<typename T, typename TArchive>
concept TypeWithSerialize = requires(T& v, TArchive& ar) { v.serialize(ar); };

template<typename T, typename TArchive>
concept TypeWithArchiveSupport = requires(T& v, uint8_t k, TArchive& ar) { ar.add(k, v); };

#else
    static_assert(false,"__cpp_concepts missing!");
#endif

template <typename T>
struct BinaryKeyValue {
    uint8_t key;
    const T& value;
    const char* name = nullptr;
};


class ArchiveError : public core::Error {
public:
    explicit ArchiveError(std::string&& msg) : core::Error(std::move(msg)) {}
};


class ArchiveOutOfKeys : public ArchiveError {
public:
    ArchiveOutOfKeys() : ArchiveError("Tried to allocate more than 16 keys per object.") {}
};


class ArchiveBadMagic : public ArchiveError {
public:
    ArchiveBadMagic() : ArchiveError("Bad magic") {}
};


class ArchiveBadVersion : public ArchiveError {
public:
    ArchiveBadVersion() : ArchiveError("Bad version") {}
};


class ArchiveBadFlags: public ArchiveError {
public:
    ArchiveBadFlags() : ArchiveError("Bad flags") {}
};


class ArchiveBadChunkType : public ArchiveError {
public:
    ArchiveBadChunkType() : ArchiveError("Bad chunk type") {}
};


class ArchiveBadChecksum : public ArchiveError {
public:
    ArchiveBadChecksum() : ArchiveError("Bad checksum") {}
};


class ArchiveUnexpectedEnd : public ArchiveError {
public:
    ArchiveUnexpectedEnd() : ArchiveError("Corrupted archive (chunk size larger than available data)") {}
};


class ArchiveMissingChecksum : public ArchiveError {
public:
    ArchiveMissingChecksum() : ArchiveError("Archive checksum not found") {}
};


template <class TImpl>
class BinaryBase {
public:
    BinaryBase() { m_group_stack.emplace_back(); }

    template<typename ...Args>
    void operator() (Args... args) {
        ((void) apply(args), ...);
    }

    template <TypeWithSerialize<TImpl> T>
    void apply(T& value) {
        uint8_t key = draw_next_key();
        static_cast<TImpl*>(this)->enter_group(key);
        value.serialize(*static_cast<TImpl*>(this));
        static_cast<TImpl*>(this)->leave_group(key);
    }

    template <TypeWithSerialize<TImpl> T>
    void apply(BinaryKeyValue<T>&& kv) {
        static_cast<TImpl*>(this)->enter_group(kv.key);
        kv.value.serialize(*static_cast<TImpl*>(this));
        static_cast<TImpl*>(this)->leave_group(kv.key);
    }

    template <TypeWithArchiveSupport<TImpl> T>
    void apply(T& value) {
        uint8_t key = draw_next_key();
        static_cast<TImpl*>(this)->add(key, value);
    }

    template <TypeWithArchiveSupport<TImpl> T>
    void apply(BinaryKeyValue<T>&& kv) {
        static_cast<TImpl*>(this)->add(kv.key, kv.value);
    }

protected:

    bool is_root_group() const {
        return m_group_stack.size() == 1;
    }

    auto& group_buffer() { return m_group_stack.back().buffer; }


    enum Header: uint8_t {
        // magic: CBDF (Chunked Binary Data Format)
        Magic0              = 0xCB,
        Magic1              = 0xDF,
        // version: '0' (first version of the format)
        Version             = 0x30,
    };

    enum Endianness: uint8_t {
        LittleEndian    = 0b00000001,
        BigEndian       = 0b00000010,
        EndiannessMask  = 0b00000011,
    };

    enum Checksum: uint8_t {
        ChecksumCNone   = 0b00000000,
        ChecksumCrc32   = 0b00000100,
        ChecksumSha256  = 0b00001000,
        ChecksumMask    = 0b00001100,
    };

    enum Type: uint8_t {
        // chunk type, upper 4 bits
        Null        =  0 << 4,
        BoolFalse   =  1 << 4,
        BoolTrue    =  2 << 4,
        Byte        =  3 << 4,
        UInt32      =  4 << 4,
        UInt64      =  5 << 4,
        Int32       =  6 << 4,
        Int64       =  7 << 4,
        Float32     =  8 << 4,
        Float64     =  9 << 4,
        Varint      = 10 << 4,
        Array       = 11 << 4,
        String      = 12 << 4,
        Binary      = 13 << 4,
        Master      = 14 << 4,
        Control     = 15 << 4,

        TypeMask    = 0xF0,
        KeyMask     = 0x0F,

        ChunkNotFound = 0xFF,
    };

    enum ControlSubtype: uint8_t {
        Metadata    = 0,
        Data        = 1,
    };

    struct Group {
        uint8_t next_key = 0;
        std::bitset<16> used_keys;
        typename TImpl::BufferType buffer {};
    };
    std::vector<Group> m_group_stack;

private:
    uint8_t draw_next_key() {
        auto& chunk = m_group_stack.back();

        // jump to next unused key
        while (chunk.next_key < 16 && chunk.used_keys.test(chunk.next_key)) {
            ++ chunk.next_key;
        }

        if (chunk.next_key == 16)
            throw ArchiveOutOfKeys();

        chunk.used_keys.set(chunk.next_key);
        return chunk.next_key ++;
    }
};


} // namespace xci::data

#endif // include guard
