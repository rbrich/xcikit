// ResourceUsage.h created on 2022-08-06 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_RESOURCEUSAGE_H
#define XCI_CORE_RESOURCEUSAGE_H

#include <chrono>
#include <cstdint>

namespace xci::core {

/// Measure resource usage, including time.
/// Uses getrusage(2) or similar.
class ResourceUsage
{
    using WallClock = std::chrono::steady_clock;
public:
    ResourceUsage() = default;
    explicit ResourceUsage(const char* name, bool start_now=true)
            : m_name(name) {  if (start_now) start(); }
    ~ResourceUsage() { stop(); }

    void start(const char* name = nullptr);
    void stop();

    void start_if(bool condition, const char* name) {
        if (condition)
            start(name);
    }

private:
    struct Measurements {
        WallClock::time_point wall;
        uint64_t user;
        uint64_t system;
        long page_faults;
        long page_reclaims;
        long blk_in;
        long blk_out;

        void reset() { wall = WallClock::time_point{}; }
        operator bool() const { return wall != WallClock::time_point{}; }
    };

    Measurements measure() const;

    const char* m_name = nullptr;
    Measurements m_start;
};

} // namespace xci::core

#endif  // include guard
