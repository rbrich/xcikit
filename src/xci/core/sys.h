// sys.h created on 2018-08-17 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_SYS_H
#define XCI_CORE_SYS_H

#include <initializer_list>
#include <filesystem>
#include <string>
#include <cstdint>
#include <csignal>
#include <ctime>

#ifndef _WIN32
#include <sys/types.h>
#endif

namespace xci::core {

namespace fs = std::filesystem;


// Get number of CPUs.
// Equivalent to:
// - Mac: sysctl -n hw.ncpu
// - Linux: grep processor /proc/cpuinfo | wc -l
int cpu_count();


/// Get number of seconds since some unspecified point in time.
/// Use only difference, for measuring intervals.
/// The timer is monotonic and does not tick when the system is asleep.
/// See: https://en.wikipedia.org/wiki/Time_Stamp_Counter
double get_cpu_time();


/// Convenience wrapper around localtime_r
inline std::tm localtime(std::time_t t)
{
    std::tm r {};
#ifdef _WIN32
    localtime_s(&r, &t);
#else
    localtime_r(&t, &r);
#endif
    return r;
}


// Integral thread ID
// The actual type is system-dependent.
#if defined(__linux__)
    using ThreadId = pid_t;
#elif defined(__EMSCRIPTEN__)
    using ThreadId = uintptr_t;
#elif defined(__APPLE__)
    using ThreadId = uint64_t;
#elif defined(_WIN32)
    using ThreadId = unsigned long;
#else
    #error "Unsupported platform"
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
fs::path home_directory_path();

/// Retrieve absolute file path of currently running process.
fs::path self_executable_path();

#ifndef _WIN32
std::string uid_to_user_name(uid_t uid);
std::string gid_to_group_name(gid_t gid);
#endif

/// Returns error message for `errno_`.
/// Calls a thread-safe variant of strerror.
std::string error_str(int errno_ = errno);

/// Returns value of GetLastError on Windows, errno elsewhere.
int last_error();

/// Returns message for GetLastError on Windows, same as error_str elsewhere.
std::string last_error_str(int err_ = last_error());

}  // namespace xci::core

#endif // XCI_CORE_SYS_H
