// buffer.h created on 2018-11-12, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
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

#ifndef XCI_CORE_BUFFER_H
#define XCI_CORE_BUFFER_H

#include <xci/compat/utility.h>
#include <functional>
#include <memory>

namespace xci::core {


// Non-owned buffer. Data ownership is not transferred.
// This could be specialization of C++20 span.
class BufferView {
public:
    BufferView(byte* data, std::size_t size)
        : m_data(data), m_size(size) {}

    byte* data() const { return m_data; }
    std::size_t size() const { return m_size; }

private:
    byte* m_data;
    std::size_t m_size;
};


// Possibly owned buffer. Attach deleter when transferring ownership.
class Buffer {
    using Deleter = std::function<void(byte*, std::size_t)>;

public:
    Buffer(byte* data, std::size_t size)
        : m_data(data), m_size(size) {}
    Buffer(byte* data, std::size_t size, Deleter deleter)
        : m_data(data), m_size(size), m_deleter(std::move(deleter)) {}
    ~Buffer() { if (m_deleter) m_deleter(m_data, m_size); }

    // non copyable, non movable
    Buffer(const Buffer &) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer &&) = delete;
    Buffer& operator=(Buffer&&) = delete;

    byte* data() const { return m_data; }
    std::size_t size() const { return m_size; }

    //std::span<byte> span() const { return {m_data, m_size}; }

private:
    byte* m_data;
    std::size_t m_size;
    Deleter m_deleter = {};
};

using BufferPtr = std::shared_ptr<const Buffer>;


} // namespace xci::core

#endif // include guard
