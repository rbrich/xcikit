// ResourceUsage.cpp created on 2022-08-06 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ResourceUsage.h"
#include <fmt/core.h>

#ifndef _WIN32
#include <sys/resource.h>
#endif

namespace xci::core {


static uint64_t timeval_to_micros(const timeval& tv)
{
    return tv.tv_sec * 1'000'000 + tv.tv_usec;
}


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
    auto end = measure();
    auto wall = std::chrono::duration_cast<std::chrono::microseconds>(end.wall - m_start.wall);
    auto usr = end.user - m_start.user;
    auto sys = end.system - m_start.system;
    auto faults = end.page_faults - m_start.page_faults;
    auto reclaims = end.page_reclaims - m_start.page_reclaims;
    auto blk_in = end.blk_in - m_start.blk_in;
    auto blk_out = end.blk_out - m_start.blk_out;
    fmt::print("⧗ {:20} {:>8} µs real {:>8} µs usr {:>8} µs sys {:>5}"
               " pg rclm {:>5} pg flt {:>5} blk in {:>5} blk out\n",
               m_name, wall.count(), usr, sys,
               reclaims, faults, blk_in, blk_out);
    m_start.reset();
}


ResourceUsage::Measurements ResourceUsage::measure() const
{
    ResourceUsage::Measurements res {};
    res.wall = WallClock::now();
#ifndef _WIN32
    struct rusage r_usage;
    if (getrusage(RUSAGE_SELF, &r_usage) == 0) {
        res.user = timeval_to_micros(r_usage.ru_utime);
        res.system = timeval_to_micros(r_usage.ru_stime);
        res.page_faults = r_usage.ru_majflt;
        res.page_reclaims = r_usage.ru_minflt;
        res.blk_in = r_usage.ru_inblock;
        res.blk_out = r_usage.ru_oublock;
    }
#endif
    return res;
}


} // namespace xci::core
