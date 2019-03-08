// types.h created on 2018-11-12, part of XCI toolkit
// Copyright 2018 Radek Brich
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

#ifndef XCI_CORE_TYPES_H
#define XCI_CORE_TYPES_H

#include <memory>
#include <functional>
#include <utility>
#include <cstddef>

namespace xci {
namespace core {


#ifdef __cpp_lib_byte
using Byte = std::byte;
#else
enum class Byte: uint8_t {};
#endif

// Possibly owned buffer. Attach deleter when transferring ownership.
class Buffer {
    using Deleter = std::function<void(Byte*, std::size_t)>;

public:
    Buffer(Byte* data, std::size_t size)
            : m_data(data), m_size(size) {}
    Buffer(Byte* data, std::size_t size, Deleter deleter)
        : m_data(data), m_size(size), m_deleter(std::move(deleter)) {}
    ~Buffer() { if (m_deleter) m_deleter(m_data, m_size); }

    // non copyable, non movable
    Buffer(const Buffer &) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer &&) = delete;
    Buffer& operator=(Buffer&&) = delete;

    Byte* data() const { return m_data; }
    std::size_t size() const { return m_size; }

    //std::span<Byte> span() const { return {m_data, m_size}; }

private:
    Byte* m_data;
    std::size_t m_size;
    Deleter m_deleter = {};
};

using BufferPtr = std::shared_ptr<const Buffer>;


}} // namespace xci::core

#endif // include guard
