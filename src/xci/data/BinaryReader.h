// BinaryReader.h created on 2019-03-14, part of XCI toolkit
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

#ifndef XCI_DATA_BINARY_READER_H
#define XCI_DATA_BINARY_READER_H

#include "BinaryBase.h"
#include <xci/data/coding/leb128.h>
#include <xci/data/reflection.h>
#include <xci/compat/endian.h>
#include <xci/compat/macros.h>

#include <istream>
#include <iterator>
#include <map>
#include <functional>

namespace xci::data {


class BinaryReader : public BinaryBase<BinaryReader> {
public:
    struct Buffer {
        //std::byte* data;
        size_t size = 0;
    };
    using BufferType = Buffer;

    explicit BinaryReader(std::istream& is) : m_stream(is) { read_header(); }

    void add(uint8_t key, std::nullptr_t) {
        const auto chunk_type = read_chunk_head(key);
        if (chunk_type != ChunkNotFound && chunk_type != Type::Null)
            throw ArchiveBadChunkType();
    }

    void add(uint8_t key, bool& value) {
        const auto chunk_type = read_chunk_head(key);
        if (chunk_type == ChunkNotFound)
            return;
        if (chunk_type == Type::BoolFalse) {
            value = false;
            return;
        }
        if (chunk_type == Type::BoolTrue) {
            value = true;
            return;
        }
        throw ArchiveBadChunkType();
    }

    template <class T> requires ( std::is_same_v<T, std::byte> || (std::is_integral_v<T> && sizeof(T) == 1) )
    void add(uint8_t key, T& value) {
        const auto chunk_type = read_chunk_head(key);
        if (chunk_type == Type::Byte)
            read_with_crc(value);
        else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }

    void add(uint8_t key, uint32_t& value) {
        const auto chunk_type = read_chunk_head(key);
        if (chunk_type == Type::UInt32)
           read_with_crc(value);
        else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }

    void add(uint8_t key, uint64_t& value) {
        const auto chunk_type = read_chunk_head(key);
        if (chunk_type == Type::UInt64)
            read_with_crc(value);
        else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }

    void add(uint8_t key, int32_t& value) {
        const auto chunk_type = read_chunk_head(key);
        if (chunk_type == Type::Int32)
            read_with_crc(value);
        else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }

    void add(uint8_t key, int64_t& value) {
        const auto chunk_type = read_chunk_head(key);
        if (chunk_type == Type::Int64)
            read_with_crc(value);
        else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }

    void add(uint8_t key, float& value) {
        const auto chunk_type = read_chunk_head(key);
        if (chunk_type == Type::Float32)
            read_with_crc(value);
        else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }

    void add(uint8_t key, double& value) {
        const auto chunk_type = read_chunk_head(key);
        if (chunk_type == Type::Float64)
            read_with_crc(value);
        else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }

    void enter_group(uint8_t key);
    void leave_group(uint8_t key);

    void finish_and_check() { read_footer(); }

/*
    template <class T, typename std::enable_if_t<meta::isRegistered<T>(), int> = 0>
    void read(T& o) {
        uint8_t type = 0;
        uint64_t len = 0;
        for (;;) {
            read_type_len(type, len);

            // Process end of member object (this is special, it has no key)
            if (type == Type_Master && len == Master_Leave) {
                --m_depth;
                break;
            }

            const auto * key = read_key();
            switch (type) {
                case Type_Master:
                    // enter member object (read a sub-object)
                    if (len == Master_Enter) {
                        ++m_depth;
                        meta::doForAllMembers<T>(
                            [this, key, &o](const auto& member) {
                                if (!strcmp(key, member.getName())) {
                                    this->read(member.getRef(o));
                                }
                            });
                    } else {
                        m_stream.setstate(std::ios::failbit);
                        m_error = Error::BadFieldType;
                        return;
                    }
                    break;
                case Type_Float:
                case Type_Integer:
                    meta::doForAllMembers<T>(
                            [this, key, &o](const auto& member) {
                                if (!strcmp(key, member.getName())) {
                                    this->read(member.getRef(o));
                                }
                            });
                    break;
                case Type_String: {
                    std::string value(len, 0);
                    read(value);
                    meta::setMemberValue<std::string>(o, key, std::move(value));
                    break;
                }
                default:
                    m_stream.setstate(std::ios::failbit);
                    m_error = Error::BadFieldType;
                    return;
            }
        }
    }
*/
    void read(std::string& value);

    template <class T, typename std::enable_if_t<meta::isRegistered<typename T::value_type>(), int> = 0>
    void read(T& out) {
        typename T::value_type value;
        read(value);
        out.push_back(value);
    }

    template <class T, typename std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value, int> = 0>
    void read(T& out) {
        unsigned int value = 0;
        read_with_crc(value);
        out = T(le32toh(value));
    }

    template <class T, typename std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
    void read(T& out) {
        double value = 0.;
        read_with_crc(value);
        out = T(value);
    }

private:
    void read_header();
    void read_footer();

    void skip_unknown_chunk(uint8_t type, uint8_t key);

    constexpr bool type_has_len(uint8_t type) {
        return type == Varint || type == Array || type == String || type == Master;
    }

    constexpr size_t size_by_type(uint8_t type) {
        switch (type) {
            case Null:
            case BoolFalse:
            case BoolTrue:
            case Control:
                return 0;
            case Byte:
                return 1;
            case UInt32:
            case Int32:
            case Float32:
                return 4;
            case UInt64:
            case Int64:
            case Float64:
                return 8;
            default:
                UNREACHABLE;
        }
    }

    uint8_t read_chunk_head(uint8_t key) {
        // Read ahead until key matches
        while (group_buffer().size != 0) {
            uint8_t b;
            read_with_crc(b);
            const uint8_t chunk_key = b & KeyMask;
            const uint8_t chunk_type = b & TypeMask;
            if (key != chunk_key) {
                skip_unknown_chunk(chunk_type, chunk_key);
                continue;
            }
            // success, pointer is at LEN or VALUE
            return chunk_type;
        }
        return ChunkNotFound;
    }

    template <typename T>
    void read_with_crc(T& value) {
        read_with_crc((std::byte*)&value, sizeof(value));
    }
    void read_with_crc(std::byte* buffer, size_t length);
    std::byte read_byte_with_crc();
    std::byte peek_byte();

    template<typename T>
    T read_leb128() {
        class ReadWithCrcIter {
        public:
            explicit ReadWithCrcIter(BinaryReader& reader) : m_reader(reader) {}
            void operator++() { m_reader.read_byte_with_crc();}
            std::byte operator*() { return m_reader.peek_byte(); }
        private:
            BinaryReader& m_reader;
        };
        auto iter = ReadWithCrcIter(*this);
        return decode_leb128<size_t>(iter);
    }

private:
    std::istream& m_stream;

    bool m_has_crc = false;
    Crc32 m_crc;

    using UnknownChunkCb = std::function<void(uint8_t type, uint8_t key, const std::byte* data, size_t size)>;
    UnknownChunkCb m_unknown_chunk_cb;
};


} // namespace xci::data

#endif // include guard
