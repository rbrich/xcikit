// SharedLibrary.cpp created on 2018-04-19, part of XCI toolkit
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

#include "SharedLibrary.h"
#include <dlfcn.h>

namespace xci {
namespace util {


bool SharedLibrary::open(const std::string& filename)
{
    m_library = dlopen(filename.c_str(), RTLD_LAZY);
    return m_library != nullptr;
}


bool SharedLibrary::close()
{
    int rc = 0;
    if (m_library) {
        rc = dlclose(m_library);
        m_library = nullptr;
    }
    return rc == 0;
}


void* SharedLibrary::resolve(const std::string& symbol)
{
    return dlsym(m_library, symbol.c_str());
}


}} // namespace xci::util
