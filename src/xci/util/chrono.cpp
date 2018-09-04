// chrono.cpp created on 2018-09-04, part of XCI toolkit
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

#include "chrono.h"
#include <ctime>

namespace xci::util {


std::chrono::system_clock::time_point localtime_now()
{
    // UTC (secs since epoch)
    time_t t = time(nullptr);
    // convert to local time struct, then present is as UTC and convert back
    t = timegm(localtime(&t));
    // t is local time (secs since "local time" epoch)
    return std::chrono::system_clock::from_time_t(t);
}


} // namespace xci::util
