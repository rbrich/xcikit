// FpsCounter.cpp created on 2018-04-13, part of XCI toolkit
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

#include "FpsCounter.h"

namespace xci::core {


void FpsCounter::tick(float frame_time)
{
    m_delta += frame_time;
    while (m_delta >= m_fraction) {
        m_delta -= m_fraction;
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
    for (auto i = (m_idx+1 == m_samples.size()) ? 0 : m_idx+1;;
              i = (i+1 == m_samples.size()) ? 0 : i+1) {
        auto& s = m_samples[i];
        if (s.num_frames > 0)
            last_sample = s.total_time / s.num_frames;
        cb(last_sample);
        if (i == m_idx)
            break;
    }
}


} // namespace xci::core
