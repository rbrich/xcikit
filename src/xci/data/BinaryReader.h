// BinaryReader.h created on 2019-03-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_DATA_BINARY_READER_H
#define XCI_DATA_BINARY_READER_H

#include "BinaryBase.h"
#include <xci/data/coding/leb128.h>
#include <xci/compat/endian.h>
#include <xci/compat/macros.h>

#include <istream>
#include <iterator>
#include <map>
#include <functional>
#include <memory>

namespace xci::data {


class BinaryReader : public ArchiveBase<BinaryReader>, BinaryBase {
    friend ArchiveBase<BinaryReader>;
    struct Buffer {
        size_t size = 0;
    };
    using Reader = std::true_type;
    using BufferType = Buffer;

public:
    explicit BinaryReader(std::istream& is) : m_stream(is) { read_header(); }

    void finish_and_check() { read_footer(); }

    // raw and smart pointers
    template <FancyPointerType T>
    void add(ArchiveField<T>&& a) {
        const auto chunk_type = peek_chunk_head(a.key);
        if (chunk_type == ChunkNotFound)
            return;
        if (chunk_type == Type::Null) {
            (void) read_chunk_head(a.key);
            a.value = nullptr;
            return;
        }
        using ElemT = typename std::pointer_traits<T>::element_type;
        a.value = new ElemT{};
        apply(ArchiveField<ElemT>{a.key, *a.value, a.name});
    }

    // bool
    void add(ArchiveField<bool>&& a) {
        const auto chunk_type = read_chunk_head(a.key);
        if (chunk_type == ChunkNotFound)
            return;
        if (chunk_type == Type::BoolFalse) {
            a.value = false;
            return;
        }
        if (chunk_type == Type::BoolTrue) {
            a.value = true;
            return;
        }
        throw ArchiveBadChunkType();
    }

    // integers, floats, enums
    template <typename T>
    requires requires() { to_chunk_type<T>(); }
    void add(ArchiveField<T>&& a) {
        const auto chunk_type = read_chunk_head(a.key);
        if (chunk_type == to_chunk_type<T>())
            read_with_crc(a.value);
        else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }

    // string
    void add(ArchiveField<std::string>&& a) {
        const auto chunk_type = read_chunk_head(a.key);
        if (chunk_type == Type::String) {
            auto length = read_leb128<size_t>();
            a.value.resize(length);
            read_with_crc((std::byte*)&a.value[0], length);
        } else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }

    // iterables
    template <typename T>
    requires requires (T& v) { v.emplace_back(); }
    void add(ArchiveField<T>&& a) {
        for (;;) {
            const auto chunk_type = peek_chunk_head(a.key);
            if (chunk_type == ChunkNotFound)
                return;
            a.value.emplace_back();
            apply(ArchiveField<typename T::value_type>{reuse_same_key(a.key), a.value.back(), a.name});
        }
    }

private:
    void enter_group(uint8_t key, const char* name);
    void leave_group(uint8_t key, const char* name);

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

    uint8_t peek_chunk_head(uint8_t key) {
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

    uint8_t read_chunk_head(uint8_t key) {
        auto chunk_type = peek_chunk_head(key);
        if (chunk_type == ChunkNotFound)
            return ChunkNotFound;
        read_byte_with_crc();
        return chunk_type;
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
        return leb128_decode<size_t>(iter);
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
