// ResourceUsage.h created on 2022-08-06 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_RESOURCEUSAGE_H
#define XCI_CORE_RESOURCEUSAGE_H

#include <cstdint>

namespace xci::core {

/// Measure resource usage, including time.
/// Uses getrusage(2) wherever available.
class ResourceUsage {
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
        uint64_t real_time;  // microseconds
        uint64_t user_time;  // microseconds
        uint64_t system_time;  // microseconds
        long max_rss;  // in kilobytes
        long page_faults;
#ifndef _WIN32
        long page_reclaims;
        long blk_in;
        long blk_out;
#endif

        void reset() { real_time = 0; }
        operator bool() const { return real_time != 0; }
    };

    Measurements measure() const;

    const char* m_name = nullptr;
    Measurements m_start {};
};

} // namespace xci::core

#endif  // include guard
