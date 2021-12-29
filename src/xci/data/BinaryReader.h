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

#include <istream>
#include <iterator>
#include <map>
#include <functional>
#include <memory>

namespace xci::data {


class BinaryReader : public ArchiveBase<BinaryReader>, protected BinaryBase {
    friend ArchiveBase<BinaryReader>;
    struct BufferType {
        size_t size = 0;        // size of group content, in bytes
        bool metadata = false;  // the next chunk in group is data or metadata?
    };

public:
    using Reader = std::true_type;
    template<typename T> using FieldType = T&;

    explicit BinaryReader(std::istream& is) : m_stream(is) { read_header(); }

    uint8_t flags() const { return m_flags; }
    bool has_crc() const { return (m_flags & ChecksumMask) == ChecksumCrc32; }
    uint32_t crc() const { return m_crc.as_uint32(); }

    // size of root group
    size_t root_group_size() const { return m_group_stack.front().buffer.size; }

    using UnknownChunkCb = std::function<void(uint8_t type, uint8_t key, const std::byte* data, size_t size)>;
    void set_unknown_chunk_callback(UnknownChunkCb cb) { m_unknown_chunk_cb = std::move(cb); }

    struct GenericNext {
        enum What {
            DataItem,
            MetadataItem,
            EnterGroup,  // Master chunk
            LeaveGroup,
            EnterMetadata,
            LeaveMetadata,
            EndOfFile,
        };
        What what;
        // chunk:
        uint8_t type = 0;
        uint8_t key = 0;
        std::unique_ptr<std::byte[]> data;
        size_t size = 0;
    };
    GenericNext generic_next();

    void finish_and_check() { skip_until_metadata(); read_footer(); }

    // raw and smart pointers
    template <FancyPointerType T>
    void add(ArchiveField<BinaryReader, T>&& a) {
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
        apply(ArchiveField<BinaryReader, ElemT>{a.key, *a.value, a.name});
    }

    // bool
    void add(ArchiveField<BinaryReader, bool>&& a) {
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
    void add(ArchiveField<BinaryReader, T>&& a) {
        const auto chunk_type = read_chunk_head(a.key);
        if (chunk_type == to_chunk_type<T>())
            read_with_crc(a.value);
        else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }

    // string
    void add(ArchiveField<BinaryReader, std::string>&& a) {
        const auto chunk_type = read_chunk_head(a.key);
        if (chunk_type == Type::String) {
            auto length = read_leb128<size_t>();
            a.value.resize(length);
            read_with_crc((std::byte*)&a.value[0], length);
        } else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }

    // iterables
    template <ContainerTypeWithEmplaceBack T>
    void add(ArchiveField<BinaryReader, T>&& a) {
        for (;;) {
            const auto chunk_type = peek_chunk_head(a.key);
            if (chunk_type == ChunkNotFound)
                return;
            a.value.emplace_back();
            apply(ArchiveField<BinaryReader, typename T::value_type>{a.key, a.value.back(), a.name});
        }
    }

    template <ContainerTypeWithEmplace T, typename... EmplaceArgs>
    void add(ArchiveField<BinaryReader, T>&& a, EmplaceArgs&&... args) {
        for (;;) {
            const auto chunk_type = peek_chunk_head(a.key);
            if (chunk_type == ChunkNotFound)
                return;
            typename T::value_type v {std::forward<EmplaceArgs...>(args...)};
            apply(ArchiveField<BinaryReader, typename T::value_type>{a.key, v, a.name});
            a.value.emplace(std::move(v));
        }
    }

private:
    void read_header();
    void read_footer();

    void enter_group(uint8_t key, const char* name);
    void leave_group(uint8_t key, const char* name);

    void skip_until_metadata();
    void skip_unknown_chunk(uint8_t type, uint8_t key);
    std::pair<std::unique_ptr<std::byte[]>, size_t> read_chunk_content(uint8_t type, uint8_t key);

    uint8_t peek_chunk_head(uint8_t key);
    uint8_t read_chunk_head(uint8_t key);

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

    std::istream& m_stream;

    uint8_t m_flags = 0;
    Crc32 m_crc;

    UnknownChunkCb m_unknown_chunk_cb;
};


} // namespace xci::data

#endif // include guard
