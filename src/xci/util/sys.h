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

#ifndef XCI_UTIL_SYS_H
#define XCI_UTIL_SYS_H

#include <cstdint>

namespace xci {
namespace util {


#if defined(__linux__)
    using ThreadId = int32_t;
#elif defined(__APPLE__)
    using ThreadId = uint64_t;
#endif

// Get thread ID
ThreadId get_thread_id();


}}  // namespace xci::util

#endif //XCI_UTIL_SYS_H
