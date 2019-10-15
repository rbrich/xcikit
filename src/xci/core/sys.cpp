// sys.cpp created on 2018-08-17, part of XCI toolkit
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

#include "sys.h"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>


#if defined(__linux__)
    #include <unistd.h>
    #include <sys/syscall.h>
#elif defined(__APPLE__)
    #include <pthread.h>
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
#else
    #error "Unsupported OS"
#endif
}



void block_signals(std::initializer_list<int> signums)
{
    sigset_t sigset;
    sigemptyset(&sigset);
    for (const auto signum : signums)
        sigaddset(&sigset, signum);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
}


int pending_signals(std::initializer_list<int> signums)
{
    sigset_t sigset;
    if (sigpending(&sigset) < 0)
        return -1;
    for (const auto signum : signums)
        if (sigismember(&sigset, signum))
            return signum;
    return 0;
}


std::string get_home_dir()
{
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
}


}  // namespace xci::core

