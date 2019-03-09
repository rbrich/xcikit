// utility.h created on 2019-03-09, part of XCI toolkit
// Copyright 2019 Radek Brich
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

#ifndef XCI_COMPAT_UTILITY_H
#define XCI_COMPAT_UTILITY_H

// This is included from <utility>, but let's keep the includes minimal.
#include <cstddef>

#if __cplusplus >= 201703L || defined(__cpp_lib_byte)
namespace xci {
    using byte = std::byte;
}
#else
namespace xci {
    enum class byte: unsigned char {};
}
#endif

#endif // include guard
