// bit_cast.h created on 2018-11-11, part of XCI toolkit
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

#ifndef XCI_COMPAT_BIT_CAST_H
#define XCI_COMPAT_BIT_CAST_H

#include <type_traits>
#include <cstring>

namespace xci::compat {


// C++20 compatibility
// https://en.cppreference.com/w/cpp/numeric/bit_cast

template <class To, class From>
typename std::enable_if<
    (sizeof(To) == sizeof(From)) &&
    std::is_trivially_copyable<From>::value &&
    std::is_trivial<To>::value,
    To>::type
bit_cast(const From &src) noexcept
{
    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}


} // namespace xci::compat

#endif // include guard
