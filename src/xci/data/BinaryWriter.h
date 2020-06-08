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
#include <ostream>

namespace xci::data {


/// Writes reflected objects to a binary stream.
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

class BinaryWriter : public BinaryBase<BinaryWriter> {
public:
    explicit BinaryWriter(std::ostream& os) : m_stream(os) { write_header(); }
    ~BinaryWriter() { terminate_content(); }

    void add(uint8_t key, std::nullptr_t) {
        write(uint8_t(Type::Null | key));
    }

    void add(uint8_t key, bool value) {
        uint8_t type = (value ? Type::BoolTrue : Type::BoolFalse);
        write(uint8_t(type | key));
    }

    template <class T> requires ( std::is_same_v<T, std::byte> || (std::is_integral_v<T> && sizeof(T) == 1) )
    void add(uint8_t key, T value) {
        write(uint8_t(Type::Byte | key));
        write(value);
    }

    void add(uint8_t key, uint32_t value) {
        write(uint8_t(Type::UInt32 | key));
        write(value);
    }

    void add(uint8_t key, uint64_t value) {
        write(uint8_t(Type::UInt64 | key));
        write(value);
    }

    void add(uint8_t key, int32_t value) {
        write(uint8_t(Type::Int32 | key));
        write(value);
    }

    void add(uint8_t key, int64_t value) {
        write(uint8_t(Type::Int64 | key));
        write(value);
    }

    void add(uint8_t key, float value) {
        write(uint8_t(Type::Float32 | key));
        write(value);
    }

    void add(uint8_t key, double value) {
        write(uint8_t(Type::Float64 | key));
        write(value);
    }

    off_t enter(uint8_t key);
    void leave(uint8_t key, off_t begin);

    void terminate_content();
    void write_crc32(uint32_t crc);

private:
    void write_header();

    template <typename T>
    void write(const T& value) {
        write((const std::byte*) &value, sizeof(value));
    }
    void write(const std::byte* buffer, size_t length);

private:
    std::ostream& m_stream;
    bool m_terminated = false;
};


} // namespace xci::data

#endif // include guard
