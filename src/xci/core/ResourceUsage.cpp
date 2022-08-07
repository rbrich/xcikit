// ResourceUsage.cpp created on 2022-08-06 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ResourceUsage.h"
#include <fmt/core.h>
#include <chrono>

#ifndef _WIN32
    #include <sys/resource.h>
    #include <sys/time.h>
#else
    #include <xci/core/bit.h>
    #include <Windows.h>
    #include <psapi.h>
#endif

namespace xci::core {


#ifndef _WIN32
static uint64_t timeval_to_micros(const struct timeval& tv)
{
    return tv.tv_sec * 1'000'000 + tv.tv_usec;
}
#else
static uint64_t filetime_to_micros(const FILETIME& ft) {
    // FILETIME is in 100ns
    return bit_copy<uint64_t>(&ft) / 10;
}
#endif

void ResourceUsage::start(const char* name)
{
    if (name)
        m_name = name;
    m_start = measure();
}


void ResourceUsage::stop()
{
    if (!m_start)
        return;
    auto m = measure();
    m.real_time -= m_start.real_time;
    m.user_time -= m_start.user_time;
    m.system_time -= m_start.system_time;
    m.page_faults -= m_start.page_faults;
#ifndef _WIN32
    m.page_reclaims -= m_start.page_reclaims;
    m.blk_in -= m_start.blk_in;
    m.blk_out -= m_start.blk_out;
    fmt::print("⧗ {:20} {:>8} µs real {:>8} µs usr {:>8} µs sys"
               " {:>5} pg flt {:>5} pg rclm {:>5} blk in {:>5} blk out\n",
               m_name, m.real_time, m.user_time, m.system_time,
               m.page_faults, m.page_reclaims, m.blk_in, m.blk_out);
#else
    fmt::print("⧗ {:20} {:>8} µs real {:>8} µs usr {:>8} µs sys {:>5} pg flt\n",
               m_name, m.real_time, m.user_time, m.system_time, m.page_faults);
#endif
    m_start.reset();
}


ResourceUsage::Measurements ResourceUsage::measure() const
{
    ResourceUsage::Measurements res {};
    using namespace std::chrono;
    res.real_time = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
#ifndef _WIN32
    struct rusage r_usage;
    if (getrusage(RUSAGE_SELF, &r_usage) == 0) {
        res.user_time = timeval_to_micros(r_usage.ru_utime);
        res.system_time = timeval_to_micros(r_usage.ru_stime);
        res.page_faults = r_usage.ru_majflt;
        res.page_reclaims = r_usage.ru_minflt;
        res.blk_in = r_usage.ru_inblock;
        res.blk_out = r_usage.ru_oublock;
    }
#else
    FILETIME creation_time, exit_time, kernel_time, user_time;
    if (GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time,
                         &kernel_time, &user_time))
    {
        res.user_time = filetime_to_micros(user_time);
        res.system_time = filetime_to_micros(kernel_time);
    }

    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        res.page_faults = pmc.PageFaultCount;
    }
#endif
    return res;
}


} // namespace xci::core
