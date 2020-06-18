// BinaryReader.h created on 2019-03-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

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
#include <memory>

namespace xci::data {


class BinaryReader : public BinaryBase<BinaryReader> {
public:
    struct Buffer {
        //std::byte* data;
        size_t size = 0;
    };
    using BufferType = Buffer;

    explicit BinaryReader(std::istream& is) : m_stream(is) { read_header(); }

    template <typename T>
    requires std::is_pointer_v<T> || requires { typename T::pointer; }
    void add(uint8_t key, T& value) {
        const auto chunk_type = peek_chunk_head(key);
        if (chunk_type == ChunkNotFound)
            return;
        if (chunk_type == Type::Null) {
            (void) read_chunk_head(key);
            value = nullptr;
            return;
        }
        using ElemT = typename std::pointer_traits<T>::element_type;
        value = new ElemT{};
        apply(BinaryKeyValue<ElemT>{key, *value});
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

    template <typename T>
    requires requires() { to_chunk_type<T>(); }
    void add(uint8_t key, T& value) {
        const auto chunk_type = read_chunk_head(key);
        if (chunk_type == to_chunk_type<T>())
            read_with_crc(value);
        else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }

    void enter_group(uint8_t key);
    void leave_group(uint8_t key);

    void finish_and_check() { read_footer(); }

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
            auto b = (uint8_t) read_byte_with_crc();
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

    uint8_t peek_chunk_head(uint8_t key) {
        // Read ahead until key matches
        while (group_buffer().size != 0) {
            auto b = (uint8_t) peek_byte();
            const uint8_t chunk_key = b & KeyMask;
            const uint8_t chunk_type = b & TypeMask;
            if (key != chunk_key) {
                (void) read_byte_with_crc();
                skip_unknown_chunk(chunk_type, chunk_key);
                continue;
            }
            // success, pointer is still at KEY/TYPE
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
