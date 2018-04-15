// FpsCounter.h created on 2018-04-13, part of XCI toolkit
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

#ifndef XCI_UTIL_FPSCOUNTER_H
#define XCI_UTIL_FPSCOUNTER_H

#include <vector>
#include <cstddef>
#include <cstdint>

namespace xci {
namespace util {


// Create simple graph of frames rendered in last second.
// Keeps information about how many frames were rendered (ticked)
// in each fraction of the second, ie. every 1/resolution seconds.
// FPS is then counted as a sum of the buffer.

class FpsCounter {
public:
    explicit FpsCounter(size_t resolution = 60);

    void tick(float delta_sec);
    int fps() const;

    // Export for FpsDisplay
    const std::vector<uint16_t>& ticks() const { return m_ticks; }
    size_t current_tick() const { return m_current; }

private:
    float m_fraction;
    std::vector<uint16_t> m_ticks;
    size_t m_current = 0;
    float m_delta = 0;
};


}} // namespace xci::util

#endif // XCI_UTIL_FPSCOUNTER_H
