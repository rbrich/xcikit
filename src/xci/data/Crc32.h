// Crc32.h created on 2020-06-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_DATA_CRC32_H
#define XCI_DATA_CRC32_H

#include <cstddef>
#include <cstdint>

namespace xci::data {


/// Running CRC-32 checksum.
///
///     Crc32 crc;
///     uint32_t r = crc(some_data);  // feed and read
///     uint32_t r = crc.as_uint32();     // just read
///
class Crc32 {
public:
    Crc32();

    template<typename T> requires std::is_pod_v<T>
    uint32_t operator() (const T& data) { feed(&data, sizeof(data)); return m_crc; }

    template<typename T> requires requires(T t) { t.data(); t.size(); }
    uint32_t operator() (const T& buffer) { feed(buffer.data(), buffer.size()); return m_crc; }

    void reset();
    void feed(const std::byte* data, size_t size);
    void feed(const char* data, size_t size) { feed(reinterpret_cast<const std::byte*>(data), size); }

    const std::byte* data() const { return reinterpret_cast<const std::byte*>(&m_crc); }
    constexpr size_t size() const { return sizeof m_crc; }

    uint32_t as_uint32() const { return m_crc; }

private:
    uint32_t m_crc;
};


} // namespace xci::data

#endif // include guard
