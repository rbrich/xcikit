// rtti.cc created on 2018-07-03, part of XCI toolkit
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

#include "rtti.h"

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#endif

#include <cstdlib>  // free

namespace xci::core {


std::string demangle_type_name(const char* name)
{
#if defined(__GNUC__) || defined(__clang__)
    int status;
    char* realname = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    if (status != 0) {
        // failure
        return name;
    }
    std::string result(realname);
    std::free(realname);
    return result;
#else
    // not mangled
    return name;
#endif
}


}  // namespace xci::core
