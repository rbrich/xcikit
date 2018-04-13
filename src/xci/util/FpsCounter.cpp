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
#include <numeric>

namespace xci {
namespace util {


FpsCounter::FpsCounter(size_t resolution)
{
    m_fraction = 1.0f / resolution;
    m_ticks.resize(resolution);
}


void xci::util::FpsCounter::tick(float delta_sec)
{
    m_delta += delta_sec;
    while (m_delta >= m_fraction) {
        m_delta -= m_fraction;
        m_current ++;
        if (m_current >= m_ticks.size())
            m_current = 0;
        m_ticks[m_current] = 0;
    }
    m_ticks[m_current] ++;
}


int FpsCounter::fps() const
{
    return std::accumulate(m_ticks.begin(), m_ticks.end(), 0);
}


}} // namespace xci::util
