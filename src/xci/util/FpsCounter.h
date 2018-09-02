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
#include <functional>
#include <cstddef>
#include <cstdint>
#include <cassert>

namespace xci {
namespace util {


// Create simple graph of frames rendered in last second.
// Keeps information about how many frames were rendered (ticked)
// in each fraction of the second, ie. every 1/resolution seconds.
// FPS is then counted as a sum of the buffer.

class FpsCounter {
public:
    FpsCounter() : FpsCounter(60) {}
    explicit FpsCounter(size_t resolution)
      : m_fraction(1.0f / resolution), m_samples(resolution)
      { assert(resolution < max_resolution); }

    static constexpr size_t max_resolution = 240;

    /// Append new frame time to the counter
    void tick(float frame_time);

    /// Number of frames in last second
    int frame_rate() const { return m_sum.num_frames; }

    /// Average frame time during last second
    float avg_frame_time() const { return m_sum.total_time / m_sum.num_frames; }

    // Export for FpsDisplay
    void foreach_sample(const std::function<void(float)>& cb) const;
    size_t resolution() const { return m_samples.size(); }

private:
    float m_fraction;
    float m_delta = 0;

    struct Sample {
        float total_time = 0.f;
        uint16_t num_frames = 0;

        void operator+=(const Sample& r) {
            total_time += r.total_time;
            num_frames += r.num_frames;
        }

        void operator-=(const Sample& r) {
            total_time -= r.total_time;
            num_frames -= r.num_frames;
        }
    };

    std::vector<Sample> m_samples;
    size_t m_idx = 0;  // current sample
    Sample m_sum;  // sum of all samples
};


}} // namespace xci::util

#endif // XCI_UTIL_FPSCOUNTER_H
