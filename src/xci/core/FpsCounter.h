// FpsCounter.h created on 2018-04-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_FPSCOUNTER_H
#define XCI_CORE_FPSCOUNTER_H

#include <vector>
#include <functional>
#include <cstddef>
#include <cstdint>
#include <cassert>

namespace xci::core {


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
    float avg_frame_time() const { return m_sum.total_time / float(m_sum.num_frames); }

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


} // namespace xci::core

#endif // XCI_CORE_FPSCOUNTER_H
