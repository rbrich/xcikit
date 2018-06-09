// View.cpp created on 2018-03-14, part of XCI toolkit
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

#include "View.h"

namespace xci {
namespace graphics {


void View::set_screen_size(Vec2u size)
{
    // Decide between vert+/hor+ depending on screen orientation.
    if (size.x < size.y) {
        // preserve screen width
        float aspect = float(size.y) / float(size.x);
        m_scalable_size = {2.0f, 2.0f * aspect};
    } else {
        // preserve screen height
        float aspect = float(size.x) / float(size.y);
        m_scalable_size = {2.0f * aspect, 2.0f};
    }

    m_screen_size = size;

    if (m_framebuffer_size.x == 0)
        m_framebuffer_size = size;
}


void View::set_framebuffer_size(Vec2u size)
{
    m_framebuffer_size = size;
}


void View::push_crop(const Rect_f& region)
{
    if (m_crop.empty())
        m_crop.push_back(region);
    else {
        m_crop.push_back(region.intersection(m_crop.back()));
    }
}


bool View::pop_refresh()
{
    bool res = m_needs_refresh;
    m_needs_refresh = false;
    return res;
}


void View::set_debug_flag(View::Debug flag, bool enabled) {
    if (has_debug_flag(flag) != enabled)
        m_debug ^= (DebugFlags) flag;
}


bool View::has_debug_flag(View::Debug flag) const {
    return bool(m_debug & (DebugFlags)flag);
}


}} // namespace xci::graphics
