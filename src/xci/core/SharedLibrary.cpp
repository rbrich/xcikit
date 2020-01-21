// SharedLibrary.cpp created on 2018-04-19 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "SharedLibrary.h"
#include <xci/core/log.h>
#include <xci/compat/dl.h>

namespace xci::core {


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


} // namespace xci::core
