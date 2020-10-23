// buffer.h created on 2018-11-12 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_BUFFER_H
#define XCI_CORE_BUFFER_H

#include <xci/compat/utility.h>
#include <functional>
#include <memory>
#include <string_view>

namespace xci::core {


/// Basically a span of bytes (a view).
class Buffer {
public:
    Buffer(byte* data, std::size_t size)
        : m_data(data), m_size(size) {}

    // non copyable, non movable
    Buffer(const Buffer &) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer &&) = delete;
    Buffer& operator=(Buffer&&) = delete;

    byte* data() const { return m_data; }
    std::size_t size() const { return m_size; }

    std::string_view string_view() const { return {(char*)m_data, m_size}; }

private:
    byte* m_data;
    std::size_t m_size;
};


/// Possibly owned buffer. Use deleter to free the buffer.
using BufferPtr = std::shared_ptr<const Buffer>;


} // namespace xci::core

#endif // include guard
