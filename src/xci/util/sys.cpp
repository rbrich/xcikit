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

#if defined(__linux__)
    #include <unistd.h>
    #include <sys/syscall.h>
#elif defined(__APPLE__)
    #include <pthread.h>
#endif

namespace xci {
namespace util {


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


}}  // namespace xci::util

