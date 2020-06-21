// BinaryWriter.h created on 2019-03-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_DATA_BINARY_WRITER_H
#define XCI_DATA_BINARY_WRITER_H

#include "BinaryBase.h"
#include <xci/compat/endian.h>
#include <xci/compat/bit.h>
#include <xci/data/coding/leb128.h>
#include <memory>
#include <ostream>

namespace xci::data {


/// Writes serializable objects to a binary stream.
///
/// The binary format is custom, see [docs](docs/data/binary_format.md].
///
/// Each serializable object must implement `serialize` method, similarly
/// to [cereal](https://uscilab.github.io/cereal/):
///
///     struct MyStruct {
///         template <class Archive>
///         void serialize(Archive& ar) {
///             ar(a, b, c);
///         }
///     };
///
/// The numeric keys are auto-assigned: a=0, b=1, c=2.
/// Maximum number of members serializable in this fashion is 16.
///
/// The keys can be assigned explicitly:
///
///     ar(XCI_DATA_ITEM(a, 0), XCI_DATA_ITEM(b, 1), XCI_DATA_ITEM(c, 2))
///
/// XCI_DATA_ITEM also assigns name to the item, which is same as the member name.
/// It can be customized as third argument: `XCI_DATA_ITEM(a, 0, "my_name")`

class BinaryWriter : public ArchiveBase<BinaryWriter>, BinaryBase {
    friend ArchiveBase<BinaryWriter>;
    using BufferType = std::vector<std::byte>;

public:
    explicit BinaryWriter(std::ostream& os, bool crc32 = false) : m_stream(os), m_crc32(crc32) {}
    ~BinaryWriter() { write_content(); }

    // raw and smart pointers
    template <typename T>
    requires std::is_pointer_v<T> || std::is_same_v<T, std::unique_ptr> || std::is_same_v<T, std::shared_ptr>
    void add(ArchiveField<T>&& a) {
        if (!a.value) {
            write(uint8_t(Type::Null | a.key));
            return;
        }
        using ElemT = typename std::pointer_traits<T>::element_type;
        apply(ArchiveField<ElemT>{a.key, *a.value, a.name});
    }

    // bool
    void add(ArchiveField<bool>&& a) {
        uint8_t type = (a.value ? Type::BoolTrue : Type::BoolFalse);
        write(uint8_t(type | a.key));
    }

    // integers, floats, enums
    template <typename T>
    requires requires() { to_chunk_type<T>(); }
    void add(ArchiveField<T>&& a) {
        write(uint8_t(to_chunk_type<T>() | a.key));
        write(a.value);
    }

    // string
    void add(ArchiveField<std::string>&& a) {
        write(uint8_t(Type::String | a.key));
        write_leb128(a.value.size());
        write((const std::byte*) a.value.data(), a.value.size());
    }

    // iterables
    template <typename T>
    requires requires { typename T::iterator; }
    void add(ArchiveField<T>&& a) {
        for (auto& item : a.value) {
            apply(ArchiveField<typename T::value_type>{reuse_same_key(a.key), item, a.name});
        }
    }

private:
    void enter_group(uint8_t key, const char* name);
    void leave_group(uint8_t key, const char* name);

    void write_content();

    template <typename T>
    void write(const T& value) {
        add_to_buffer((const std::byte*) &value, sizeof(value));
    }
    void write(const std::byte* buffer, size_t length) {
        add_to_buffer(buffer, length);
    }

    void add_to_buffer(const std::byte* data, size_t size) {
        group_buffer().insert(group_buffer().end(), data, data + size);
    }

    template<typename T>
    void write_leb128(T value) {
        auto out_iter = std::back_inserter(group_buffer());
        encode_leb128<T, decltype(out_iter), std::byte>(out_iter, value);
    }

private:
    std::ostream& m_stream;
    bool m_crc32;  // enable CRC32
};


} // namespace xci::data

#endif // include guard
