// backtrace.cpp created on 2025-03-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

// Possibly: binutils/addr2line.c https://sourceware.org/git/?p=binutils-gdb.git;a=blob;f=binutils/addr2line.c;hb=HEAD

#include "backtrace.h"
#include <xci/compat/unistd.h>
#include <xci/core/rtti.h>
#include <xci/core/sys.h>

#include <fmt/format.h>

#include <filesystem>
#include <unordered_set>

#ifdef _WIN32
#include <dbghelp.h>
#else
#include <execinfo.h>
#include <dlfcn.h>
#endif


namespace xci::core {

namespace fs = std::filesystem;


std::string get_backtrace(bool file_lines)
{
    constexpr size_t c_max_frames = 128;
    void* callstack[c_max_frames];
    std::string res;
#ifndef _WIN32
    const int frames = ::backtrace(callstack, c_max_frames);

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

#else
    unsigned short frames = CaptureStackBackTrace( 0, c_max_frames, callstack, nullptr);

    HANDLE process = GetCurrentProcess();

    if (!SymInitialize(process, nullptr, TRUE))
        return fmt::format("SymInitialize failed: {}", last_error_str());

    constexpr size_t max_name = 255;
    alignas(SYMBOL_INFO) char sym_buffer[sizeof(SYMBOL_INFO) + max_name + 1] {};
    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(sym_buffer);
    symbol->MaxNameLen = max_name;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    IMAGEHLP_MODULE module_info = { .SizeOfStruct = sizeof(IMAGEHLP_MODULE) };
    std::unordered_set<intptr_t> module_seen;
    std::string modules_res;

    IMAGEHLP_LINE line_info = { .SizeOfStruct = sizeof(IMAGEHLP_LINE) };
    DWORD line_displacement;

    for (int i = 0; i < frames; i++) {
        intptr_t addr = reinterpret_cast<intptr_t>(callstack[i]);
        bool have_sym = SymFromAddr(process, addr, nullptr, symbol);
        bool have_mod = SymGetModuleInfo(process, addr, &module_info);
        bool have_line = file_lines ? SymGetLineFromAddr(process, addr, &line_displacement, &line_info) : false;

        res += fmt::format("{:<3} {:<20} 0x{:x} {} + {} ({})\n",
                i, have_mod ? module_info.ModuleName : "unknown",
                addr,
                have_sym ? symbol->Name : fmt::format("0x{:x}", module_info.BaseOfImage),
                addr - intptr_t(have_sym ? symbol->Address : module_info.BaseOfImage),
                have_line ? fmt::format("{}:{}", line_info.FileName, line_info.LineNumber)
                          : fmt::format("0x{:x}", addr - module_info.BaseOfImage));

        if (have_mod && module_seen.insert(module_info.BaseOfImage).second) {
            modules_res += fmt::format("0x{:x} 0x{:x}   {}\n",
                module_info.BaseOfImage,
                module_info.BaseOfImage + module_info.ImageSize, module_info.LoadedImageName);
        }
    }
    SymCleanup(process);
    res += modules_res;
#endif

    // Remove trailing newline
    if (!res.empty()) {
        res.pop_back();
    }
    return res;
}


} // namespace xci::core
