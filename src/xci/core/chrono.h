// chrono.h created on 2018-09-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_CHRONO_H
#define XCI_CORE_CHRONO_H

// <chrono> extras

#include <chrono>

namespace xci::core {

// You get these with xci::core as a bonus!
using namespace std::chrono_literals;


std::chrono::system_clock::time_point localtime_now();


} // namespace xci::core



#endif //XCI_CORE_CHRONO_H
