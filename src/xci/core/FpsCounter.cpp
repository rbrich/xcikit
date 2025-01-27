// FpsCounter.cpp created on 2018-04-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "FpsCounter.h"

namespace xci::core {


void FpsCounter::tick(float frame_time)
{
    m_delta += frame_time;
    while (m_delta >= fraction) {
        m_delta -= fraction;
        m_idx ++;
        if (m_idx >= m_samples.size())
            m_idx = 0;
        m_sum -= m_samples[m_idx];
        m_samples[m_idx] = {0.f, 0};
    }
    m_samples[m_idx] += {frame_time, 1};
    m_sum += {frame_time, 1};
}


void FpsCounter::foreach_sample(const std::function<void(float)>& cb) const
{
    float last_sample = 0.f;
    for (auto i = m_idx;;) {
        const auto& s = m_samples[i];
        if (s.num_frames > 0)
            last_sample = s.total_time / float(s.num_frames);
        cb(last_sample);
        ++i;
        if (i == m_samples.size())
            i = 0;
        if (i == m_idx)
            break;
    }
}


} // namespace xci::core
