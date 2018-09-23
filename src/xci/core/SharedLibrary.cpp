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
#include <xci/core/log.h>
#include <dlfcn.h>

namespace xci {
namespace core {


bool SharedLibrary::open(const std::string& filename)
{
    m_library = dlopen(filename.c_str(), RTLD_LAZY);
    if (m_library == nullptr) {
        log_error("SharedLibrary: dlopen: {}", dlerror());
        return false;
    }
    return true;
}


bool SharedLibrary::close()
{
    if (m_library == nullptr)
        return true;  // already closed

    if (dlclose(m_library) != 0) {
        log_error("SharedLibrary: dlclose: {}", dlerror());
        return false;
    }

    m_library = nullptr;
    return true;
}


void* SharedLibrary::resolve(const std::string& symbol)
{
    void* res = dlsym(m_library, symbol.c_str());
    if (res == nullptr) {
        log_error("SharedLibrary: dlsym({}): {}", symbol, dlerror());
        return nullptr;
    }
    return res;
}


}} // namespace xci::core
