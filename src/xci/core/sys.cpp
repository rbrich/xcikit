// sys.cpp created on 2018-08-17 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "sys.h"
#include <xci/compat/macros.h>

// get_thread_id
#if defined(__linux__)
    #include <sys/syscall.h>
#elif defined(__APPLE__)
    #include <pthread.h>
#elif defined(_WIN32)
    #include <windows.h>
#endif

// get_home_dir, *_signals
#ifdef _WIN32
    #include <ShlObj.h>
    #include <cassert>
#else
    #include <sys/types.h>
    #include <pwd.h>
    #include <unistd.h>
#endif

namespace xci::core {


ThreadId get_thread_id()
{
#if defined(__linux__)
    return (ThreadId) syscall(SYS_gettid);
#elif defined(__APPLE__)
    uint64_t tid = 0;
    pthread_threadid_np(pthread_self(), &tid);
    return tid;
#elif defined(_WIN32)
    return GetCurrentThreadId();
#else
    #error "Unsupported OS"
#endif
}



void block_signals(std::initializer_list<int> signums)
{
#ifndef _WIN32
    sigset_t sigset;
    sigemptyset(&sigset);
    for (const auto signum : signums)
        sigaddset(&sigset, signum);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
#else
    UNUSED signums;
    assert(!"block_signals: Not implemented");
#endif
}


int pending_signals(std::initializer_list<int> signums)
{
#ifndef _WIN32
    sigset_t sigset;
    if (sigpending(&sigset) < 0)
        return -1;
    for (const auto signum : signums)
        if (sigismember(&sigset, signum))
            return signum;
    return 0;
#else
    UNUSED signums;
    assert(!"block_signals: Not implemented");
    return 0;
#endif
}


std::string get_home_dir()
{
#ifdef _WIN32
    std::string homedir(MAX_PATH, '\0');
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_PROFILE, nullptr, 0, homedir.data()))) {
        homedir.resize(strlen(homedir.data()));
    } else {
        homedir.clear();
    }
    homedir.shrink_to_fit();
    return homedir;
#else
    struct passwd pwd {};
    struct passwd *result;
    constexpr size_t bufsize = 16384;
    char buf[bufsize];

    getpwuid_r(getuid(), &pwd, buf, bufsize, &result);
    if (result == nullptr) {
        // not found or error
        // (we could check return value + errno and log error here)
        return "/tmp";
    }

    return {result->pw_dir};
#endif
}


}  // namespace xci::core

