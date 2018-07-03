// rtti.h created on 2018-07-03, part of XCI toolkit
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

#ifndef XCI_UTIL_RTTI_H
#define XCI_UTIL_RTTI_H

#include <string>
#include <typeinfo>

namespace xci {
namespace util {

// Demangle type name when mangled (GCC/Clang)
std::string demangle_type_name(const char* name);

// Returns human-readable type name for given typeid()
inline std::string type_name(const std::type_info& ti) { return demangle_type_name(ti.name()); }

}}  // namespace xci::util

#endif // XCI_UTIL_RTTI_H
