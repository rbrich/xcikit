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

namespace detail {

struct BinaryReaderBufferType {
    size_t size = 0;        // size of group content, in bytes
    bool metadata = false;  // the next chunk in group is data or metadata?
};

class BinaryReaderImpl: protected ArchiveGroupStack<BinaryReaderBufferType>, protected BinaryBase {
public:
    explicit BinaryReaderImpl(std::istream& is) : m_stream(is) { read_header(); }

    uint8_t flags() const { return m_flags; }
    bool has_crc() const { return (m_flags & ChecksumMask) == ChecksumCrc32; }
    uint32_t crc() const { return m_crc.as_uint32(); }

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

protected:
    void read_header();
    void read_footer();

    void _enter_group(uint8_t key);
    void _leave_group();

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
            explicit ReadWithCrcIter(BinaryReaderImpl& reader) : m_reader(reader) {}
            void operator++() { m_reader.read_byte_with_crc();}
            std::byte operator*() { return m_reader.peek_byte(); }
        private:
            BinaryReaderImpl& m_reader;
        };
        auto iter = ReadWithCrcIter(*this);
        return leb128_decode<size_t>(iter);
    }

private:
    std::istream& m_stream;

    uint8_t m_flags = 0;
    Crc32 m_crc;

    UnknownChunkCb m_unknown_chunk_cb;
};

}  // namespace detail


template <class TImpl>
class BinaryReaderBase : public ArchiveBase<TImpl>, public detail::BinaryReaderImpl {
    friend ArchiveBase<TImpl>;

public:
    using Reader = std::true_type;
    template<typename T> using FieldType = T&;

    explicit BinaryReaderBase(std::istream& is) : detail::BinaryReaderImpl(is) {}

    // size of root group
    size_t root_group_size() const { return m_group_stack.front().buffer.size; }

    // raw and smart pointers
    template <FancyPointerType T>
    void add(ArchiveField<TImpl, T>&& a) {
        const auto chunk_type = peek_chunk_head(a.key);
        if (chunk_type == ChunkNotFound)
            return;
        if (chunk_type == Type::Null) {
            (void) read_chunk_head(a.key);
            a.value = nullptr;
            return;
        }
        using ElemT = typename std::pointer_traits<T>::element_type;
        a.value = T(new ElemT{});
        ArchiveBase<TImpl>::apply(ArchiveField<TImpl, ElemT>{a.key, *a.value, a.name});
    }

    // bool
    void add(ArchiveField<TImpl, bool>&& a) {
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
    void add(ArchiveField<TImpl, T>&& a) {
        const auto chunk_type = read_chunk_head(a.key);
        if (chunk_type == to_chunk_type<T>())
            read_with_crc(a.value);
        else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }

    // string
    void add(ArchiveField<TImpl, std::string>&& a) {
        const auto chunk_type = read_chunk_head(a.key);
        if (chunk_type == Type::String) {
            auto length = read_leb128<size_t>();
            a.value.resize(length);
            read_with_crc((std::byte*)(a.value.data()), length);
        } else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }
    void add(ArchiveField<TImpl, const char*>&& a) {
        throw ArchiveCannotReadCString();
    }

    // binary data
    template <BlobType T>
    void add(ArchiveField<TImpl, T>&& a) {
        const auto chunk_type = read_chunk_head(a.key);
        if (chunk_type == Type::Binary) {
            a.value.resize(read_leb128<size_t>());
            read_with_crc((std::byte*) a.value.data(), a.value.size());
        } else if (chunk_type != ChunkNotFound)
            throw ArchiveBadChunkType();
    }

    // iterables
    template <ContainerTypeWithEmplaceBack T>
    void add(ArchiveField<TImpl, T>&& a) {
        for (;;) {
            const auto chunk_type = peek_chunk_head(a.key);
            if (chunk_type == ChunkNotFound)
                return;
            a.value.emplace_back();
            ArchiveBase<TImpl>::apply(ArchiveField<TImpl, typename T::value_type>{a.key, a.value.back(), a.name});
        }
    }

    template <ContainerType T, typename F>
    void add(ArchiveField<TImpl, T>&& a, F&& f_make_value) {
        for (;;) {
            const auto chunk_type = peek_chunk_head(a.key);
            if (chunk_type == ChunkNotFound)
                return;
            auto&& v = f_make_value(a.value);
            ArchiveBase<TImpl>::apply(ArchiveField<TImpl, decltype(v)>{a.key, v, a.name});
        }
    }

    // variant
    template <VariantType T>
    void add(ArchiveField<TImpl, T>&& a) {
        // index of active alternative
        size_t index = std::variant_npos;
        ArchiveBase<TImpl>::apply(ArchiveField<TImpl, size_t>{draw_next_key(a.key), index, a.name});
        a.value = variant_from_index<T>(index);
        // value of the alternative
        a.key = draw_next_key(key_auto);
        std::visit([this, &a](auto& value) {
            using Tv = std::decay_t<decltype(value)>;
            if constexpr (!std::is_same_v<Tv, std::monostate>) {
                this->ArchiveBase<TImpl>::apply(ArchiveField<TImpl, Tv>{a.key, value});
            }
        }, a.value);
    }

private:
    template <typename T>
    bool enter_group(const ArchiveField<TImpl, T>& kv) {
        _enter_group(kv.key);
        return true;
    }
    template <typename T>
    void leave_group(const ArchiveField<TImpl, T>& kv) {
        _leave_group();
    }
};


// Mimic this class to extend the reader, e.g. add your own data for use by load() function
// (You have to inherit from BinaryReaderBase with CRTP, not from BinaryReader itself.)
class BinaryReader : public BinaryReaderBase<BinaryReader> {
public:
    explicit BinaryReader(std::istream& is) : BinaryReaderBase(is) {}
};


} // namespace xci::data

#endif // include guard
