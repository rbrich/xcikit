// View.cpp created on 2018-03-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "View.h"
#include "Window.h"
#include <cassert>

namespace xci::graphics {


std::array<float, 16> View::projection_matrix() const
{
    float xs = 2.0f / viewport_size().x.value;
    float ys = 2.0f / viewport_size().y.value;
    float xt = offset().x.value * xs;
    float yt = offset().y.value * ys;
    if (m_origin == ViewOrigin::TopLeft) {
        xt -= 1.0;
        yt -= 1.0;
    }
    return {{
            xs,   0.0f, 0.0f, 0.0f,
            0.0f, ys,  0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            xt,   yt,  0.0f, 1.0f,
    }};
}


bool View::set_screen_size(ScreenCoords size)
{
    bool changed = (m_screen_size != size);
    m_screen_size = size;

    // Set framebuffer to same size, if not set explicitly (by set_framebuffer_size)
    if (m_framebuffer_size.x.value == 0.0) {
        m_framebuffer_size.x.value = size.x.value;
        m_framebuffer_size.y.value = size.y.value;
        changed = true;
    }

    if (changed || m_viewport_size.x.value == 0.0) {
        rescale_viewport();
        changed = true;
    }

    return changed;
}


bool View::set_framebuffer_size(FramebufferCoords size)
{
    bool changed = (m_framebuffer_size != size);
    m_framebuffer_size = size;
    return changed;
}


void View::set_viewport_mode(ViewOrigin origin, ViewScale scale)
{
    assert(m_crop.empty());
    assert(m_offset.empty());
    m_origin = origin;
    m_scale = scale;
    rescale_viewport();
}


ViewportCoords View::viewport_center() const
{
    if (m_origin == ViewOrigin::TopLeft) {
        return m_viewport_size / ViewportUnits{2.0};
    } else {
        return {0, 0};
    }
}


void View::push_offset(const ViewportCoords& offset)
{
    m_offset.push_back(this->offset() + offset);
}


const ViewportCoords& View::offset() const
{
    if (m_offset.empty()) {
        static ViewportCoords default_offset{0, 0};
        return default_offset;
    }
    return m_offset.back();
}


void View::push_crop(const ViewportRect& region)
{
    if (m_crop.empty())
        m_crop.push_back(region.moved(offset()));
    else {
        m_crop.push_back(region.moved(offset()).intersection(m_crop.back()));
    }
}


bool View::pop_refresh()
{
    bool res = m_needs_refresh;
    m_needs_refresh = false;
    return res;
}


void View::finish_draw()
{
    window()->finish_draw();
}


void View::set_debug_flag(View::Debug flag, bool enabled) {
    if (has_debug_flag(flag) != enabled)
        m_debug ^= (DebugFlags) flag;
}


bool View::has_debug_flag(View::Debug flag) const {
    return bool(m_debug & (DebugFlags)flag);
}


void View::rescale_viewport()
{
    switch (m_scale) {
        case ViewScale::ScalingWithAspectCorrection:
            // Decide between vert+/hor+ depending on screen orientation.
            if (m_screen_size.x < m_screen_size.y) {
                // preserve screen width
                float aspect = float(m_screen_size.y.value) / float(m_screen_size.x.value);
                m_viewport_size = {2.0f, 2.0f * aspect};
            } else {
                // preserve screen height
                float aspect = float(m_screen_size.x.value) / float(m_screen_size.y.value);
                m_viewport_size = {2.0f * aspect, 2.0f};
            }
            break;

        case ViewScale::FixedScreenPixels:
            m_viewport_size = {m_screen_size.x.value, m_screen_size.y.value};
            break;
    }
}


} // namespace xci::graphics
