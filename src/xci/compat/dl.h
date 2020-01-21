// dl.h created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_COMPAT_DL_H
#define XCI_COMPAT_DL_H

#ifndef _WIN32
#include <dlfcn.h>
#else

#include <windows.h>

#define RTLD_LAZY 0

inline void *dlopen(const char *filename, int flags) {
    (void) flags;
    return (void*) LoadLibrary(filename);
}

inline int dlclose(void *handle) {
    return !FreeLibrary((HMODULE) handle);
}

inline void *dlsym(void *handle, const char *symbol) {
    return (void*) GetProcAddress((HMODULE) handle, symbol);
}

inline const char *dlerror() {
    auto msg_id = ::GetLastError();
    if (!msg_id)
        return nullptr;  // no error

    thread_local char buffer[1000];
    auto size = FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, msg_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            buffer, sizeof(buffer), nullptr);
    if (!size) {
        snprintf(buffer, sizeof(buffer), "unknown error (%lu)", msg_id);
    }

    return buffer;
}

#endif  // _WIN32

#endif // include guard
