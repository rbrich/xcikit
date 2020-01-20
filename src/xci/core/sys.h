// sys.h created on 2018-08-17, part of XCI toolkit
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

#ifndef XCI_CORE_SYS_H
#define XCI_CORE_SYS_H

#include <initializer_list>
#include <string>
#include <cstdint>
#include <csignal>

#ifdef __linux__
#include <sys/types.h>
#endif

namespace xci::core {

// Integral thread ID
// The actual type is system-dependent.
#if defined(__linux__)
    using ThreadId = pid_t;
#elif defined(__APPLE__)
    using ThreadId = uint64_t;
#elif defined(WIN32)
    using ThreadId = unsigned long;
#endif

/// Get integral thread ID of this thread
///
/// The standard function `std::this_thread::get_id()` returns opaque object,
/// which may be dumped into iostream. Unfortunately, the implementation I tested
/// returns what seems to be pointer value as returned by `pthread_self()`.
///
/// Let's test with three threads (main + two spawned):
/// - Linux (libstdc++): `7f5cb868d740`, `7f5cb6c43700`, `7f5cb7444700` (with std::hex)
/// - Mac (libc++): `0x7fffb4263380`, `0x700005ab1000`, `0x700005b34000`
///
/// This function, on the other hand, returns real TID, eg:
/// - Linux: `13543`, `13577`, `13578`
/// - Mac: `7094490`, `7094491`, `7094492`
///
/// This, similarly to PID, is system-wide ID used to identify the thread.
/// Such value is more useful for logging and debugging purposes.
///
/// \return system-wide unique integral thread ID
ThreadId get_thread_id();


/// Block a set of signals. Blocked signals can be checked using `pending_signals`.
/// \param signums      set of signals to block
void block_signals(std::initializer_list<int> signums);

/// Check for any pending signal from the set.
/// \param signums      set of signals to check
/// \return             -1 error, 0 none, N = signum
int pending_signals(std::initializer_list<int> signums);


/// Retrieve home dir of current user from password file (i.e. /etc/passwd).
/// \return             the home dir or in case of error "/tmp"
std::string get_home_dir();


}  // namespace xci::core

#endif // XCI_CORE_SYS_H
