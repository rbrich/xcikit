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

#include <absl/types/span.h>
#include <memory>
#include <cstddef>

namespace xci {
namespace core {


#ifdef __cpp_lib_byte
using Byte = std::byte;
#else
enum class Byte: uint8_t {};
#endif

// This is an unowned buffer
using Buffer = absl::Span<Byte>;

// Possibly owned buffer. Attach deleter when transferring ownership.
using BufferPtr = std::shared_ptr<const Buffer>;


}} // namespace xci::core

#endif // include guard
