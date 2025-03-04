// backtrace.cpp created on 2025-03-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "backtrace.h"
#include <xci/compat/unistd.h>
#include <xci/core/rtti.h>

#include <fmt/format.h>

#include <filesystem>

#include <execinfo.h>
#include <dlfcn.h>
#include <unordered_set>


namespace xci::core {

namespace fs = std::filesystem;


std::string get_backtrace()
{
    constexpr size_t c_max_frames = 128;
    void* callstack[c_max_frames];
    const int frames = ::backtrace(callstack, c_max_frames);
    std::string res;

    for (int i = 0; i != frames; ++i) {
        Dl_info info;
        if (::dladdr(callstack[i], &info) != 0) {
            res += fmt::format("{:<3} {:<20} 0x{:x} {} + {} (0x{:x})\n",
                i, fs::path(info.dli_fname).filename().string(),
                intptr_t(callstack[i]),
                info.dli_sname ? demangle(info.dli_sname) : fmt::format("0x{:x}", intptr_t(info.dli_fbase)),
                intptr_t(callstack[i]) - intptr_t(info.dli_saddr ? info.dli_saddr : info.dli_fbase),
                intptr_t(callstack[i]) - intptr_t(info.dli_fbase));
        }
    }

    // List images that appeared in the backtrace
    std::unordered_set<intptr_t> seen;
    for (int i = 0; i != frames; ++i) {
        Dl_info info;
        if (::dladdr(callstack[i], &info) != 0 && seen.insert(intptr_t(info.dli_fbase)).second) {
            res += fmt::format("0x{:x}   {}\n", intptr_t(info.dli_fbase), info.dli_fname);
        }
    }

    // Remove trailing newline
    if (!res.empty()) {
        res.pop_back();
    }
    return res;
}


} // namespace xci::core
