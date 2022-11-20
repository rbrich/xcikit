// BinaryWriter.h created on 2019-03-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_DATA_BINARY_WRITER_H
#define XCI_DATA_BINARY_WRITER_H

#include "BinaryBase.h"
#include <xci/compat/endian.h>
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
///             ar(a)(b)(c);
///         }
///     };
///
/// Alternatively, it can implement save/load pair, so it can behave specifically
/// depending on the data direction:
///
///     struct MyStruct {
///         template <class Archive> void save(Archive& ar) const { ar(a)(b)(c); }
///         template <class Archive> void load(Archive& ar) { ar(a)(b)(c); }
///     };
///
/// Out-of-class functions are also supported:
///
///     template <class Archive> void serialize(Archive& ar, MyStruct& v) { ar(v.a)(v.b)(v.c); }
///     template <class Archive> void save(Archive& ar, const MyStruct& v) { ar(v.a)(v.b)(v.c); }
///     template <class Archive> void load(Archive& ar, MyStruct& v) { ar(v.a)(v.b)(v.c); }
///
/// The numeric keys are auto-assigned: a=0, b=1, c=2.
/// Maximum number of members serializable in this fashion is 16.
///
/// The keys can be assigned explicitly:
///
///     ar (0, a) (1, b) (2, c);
///     ar (0, "a", a) (1, "a", b) (2, "a", c);
///     XCI_ARCHIVE_FIELD(ar, 0, a); XCI_ARCHIVE_FIELD(ar, 1, b); XCI_ARCHIVE_FIELD(ar, 2, c);
///
/// The second line also assigns a name to each field. The third line is equivalent
/// to the second, but XCI_ARCHIVE_FIELD assigns the name automatically as stringified variable name.
///
/// As a shortcut, multiple auto-named and auto-keyed fields can be added at once:
///
///     XCI_ARCHIVE(ar, a, b, c)
///

class BinaryWriter
        : public ArchiveBase<BinaryWriter>
        , protected ArchiveGroupStack< /*BufferType=*/ std::vector<std::byte> >
        , protected BinaryBase {
    friend ArchiveBase<BinaryWriter>;

public:
    using Writer = std::true_type;
    template<typename T> using FieldType = const T&;

    explicit BinaryWriter(std::ostream& os, bool crc32 = false) : m_stream(os), m_crc32(crc32) {}
    ~BinaryWriter() { write_content(); }

    // raw and smart pointers
    template <FancyPointerType T>
    void add(ArchiveField<BinaryWriter, T>&& a) {
        if (!a.value) {
            write(uint8_t(Type::Null | a.key));
            return;
        }
        using ElemT = typename std::pointer_traits<T>::element_type;
        apply(ArchiveField<BinaryWriter, ElemT>{a.key, *a.value, a.name});
    }

    // bool
    void add(ArchiveField<BinaryWriter, bool>&& a) {
        uint8_t type = (a.value ? Type::BoolTrue : Type::BoolFalse);
        write(uint8_t(type | a.key));
    }

    // integers, floats, enums
    template <typename T>
    requires requires() { BinaryBase::to_chunk_type<T>(); }
    void add(ArchiveField<BinaryWriter, T>&& a) {
        write(uint8_t(BinaryBase::to_chunk_type<T>() | a.key));
        write(a.value);
    }

    // string
    void add(ArchiveField<BinaryWriter, std::string>&& a) {
        write(uint8_t(Type::String | a.key));
        write_leb128(a.value.size());
        write((const std::byte*) a.value.data(), a.value.size());
    }
    void add(ArchiveField<BinaryWriter, const char*>&& a) {
        if (!a.value) {
            write(uint8_t(Type::Null | a.key));
            return;
        }
        write(uint8_t(Type::String | a.key));
        auto size = strlen(a.value);
        write_leb128(size);
        write((const std::byte*) a.value, size);
    }

    // binary data
    template <BlobType T>
    void add(ArchiveField<BinaryWriter, T>&& a) {
        write(uint8_t(Type::Binary | a.key));
        write_leb128(a.value.size());
        write((const std::byte*) a.value.data(), a.value.size());
    }

    // iterables
    template <ContainerType T>
    void add(ArchiveField<BinaryWriter, T>&& a) {
        for (auto& item : a.value) {
            apply(ArchiveField<BinaryWriter, typename T::value_type>{a.key, item, a.name});
        }
    }

    // variant
    template <VariantType T>
    void add(ArchiveField<BinaryWriter, T>&& a) {
        // index of active alternative
        apply(ArchiveField<BinaryWriter, size_t>{draw_next_key(a.key), a.value.index(), a.name});
        // value of the alternative
        a.key = draw_next_key(key_auto);
        std::visit([this, &a](const auto& value) {
            using Tv = std::decay_t<decltype(value)>;
            if constexpr (!std::is_same_v<Tv, std::monostate>) {
                this->apply(ArchiveField<BinaryWriter, Tv>{a.key, value});
            }
        }, a.value);
    }

private:
    template <typename T>
    bool enter_group(const ArchiveField<BinaryWriter, T>&) {
        m_group_stack.emplace_back();
        return true;
    }
    template <typename T>
    void leave_group(const ArchiveField<BinaryWriter, T>& kv) {
        write_group(kv.key, kv.name);
    }

    void write_group(uint8_t key, const char* name);
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
        leb128_encode<T, decltype(out_iter), std::byte>(out_iter, value);
    }

private:
    std::ostream& m_stream;
    bool m_crc32;  // enable CRC32
};


} // namespace xci::data

#endif // include guard
