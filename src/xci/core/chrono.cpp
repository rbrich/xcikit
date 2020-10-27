// chrono.cpp created on 2018-09-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "chrono.h"
#include <ctime>

#ifdef _WIN32
#define timegm _mkgmtime
#endif

namespace xci::core {


std::chrono::system_clock::time_point localtime_now()
{
    // UTC (secs since epoch)
    time_t t = time(nullptr);
    // convert to local time struct, then present it as UTC and convert back
    t = timegm(localtime(&t));
    // t is local time (secs since "local time" epoch)
    return std::chrono::system_clock::from_time_t(t);
}


} // namespace xci::core
