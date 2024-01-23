// View.cpp created on 2018-03-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "View.h"
#include "Window.h"

#include <cassert>

namespace xci::graphics {


VariUnits::Type VariUnits::type() const
{
    // three upper bits, complemented in case of negative value (sign bit = 0)
    switch ( (m_storage < 0 ? ~m_storage : m_storage) >> 29 ) {
        case 0b00:  return Framebuffer;
        case 0b01:  return Screen;
        default:    return Viewport;
    }
}


FramebufferPixels VariUnits::as_framebuffer() const
{
    assert(type() == Framebuffer);
    return { float(m_storage) / float(1<<10) };
}


ScreenPixels VariUnits::as_screen() const
{
    assert(type() == Screen);
    return { float(m_storage ^ 0x20000000) / float(1<<10) };
}


ViewportUnits VariUnits::as_viewport() const
{
    assert(type() == Viewport);
    return { float(m_storage ^ 0x40000000) / float(1<<16) };
}


int32_t VariUnits::to_storage(FramebufferPixels fb)
{
    const auto r = int32_t(fb.value * float(1<<10));
    assert((r < 0 ? ~r : r) >> 29 == 0);
    return r;
}


int32_t VariUnits::to_storage(ScreenPixels px)
{
    const auto r = int32_t(px.value * float(1<<10));
    assert((r < 0 ? ~r : r) >> 29 == 0);
    return r ^ 0x20000000;
}


int32_t VariUnits::to_storage(ViewportUnits vp)
{
    const auto r = int32_t(vp.value * float(1<<16));
    assert((r < 0 ? ~r : r) >> 30 == 0);
    return r ^ 0x40000000;
}


VariUnits VariUnits::mul(float v)
{
    switch (type()) {
        case VariUnits::Framebuffer: return v * as_framebuffer();
        case VariUnits::Screen: return v * as_screen();
        case VariUnits::Viewport: return v * as_viewport();
    }
    XCI_UNREACHABLE;
}


std::ostream& operator<<(std::ostream& s, VariUnits rhs)
{
    switch (rhs.type()) {
        case VariUnits::Framebuffer: return s << rhs.as_framebuffer() << "fb";
        case VariUnits::Screen: return s << rhs.as_screen() << "px";
        case VariUnits::Viewport: return s << rhs.as_viewport() << "vp";
    }
    XCI_UNREACHABLE;
}


Mat4f View::projection_matrix() const
{
    const float xs = 2.0f / framebuffer_size().x.value;
    const float ys = 2.0f / framebuffer_size().y.value;
    float xt = offset().x.value * xs;
    float yt = offset().y.value * ys;
    if (m_origin == ViewOrigin::TopLeft) {
        xt -= 1.0;
        yt -= 1.0;
    }
    return Mat4f::scale({xs, ys, 1.0f}, {xt, yt, 0.0f});
}


void View::set_origin(ViewOrigin origin)
{
    assert(m_crop.empty());
    assert(m_offset.empty());
    m_origin = origin;
}


bool View::set_screen_size(ScreenSize size)
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


ScreenCoords View::screen_center() const
{
    if (m_origin == ViewOrigin::TopLeft) {
        return 0.5f * m_screen_size;
    } else {
        return {0, 0};
    }
}


ScreenCoords View::screen_top_left(ScreenCoords offset) const
{
    if (m_origin == ViewOrigin::TopLeft) {
        return offset;
    } else {
        return offset - 0.5f * m_screen_size;
    }
}


bool View::set_framebuffer_size(FramebufferSize size)
{
    bool changed = (m_framebuffer_size != size);
    m_framebuffer_size = size;
    return changed;
}


FramebufferCoords View::framebuffer_center() const
{
    if (m_origin == ViewOrigin::TopLeft) {
        return 0.5f * m_framebuffer_size;
    } else {
        return {0, 0};
    }
}


FramebufferCoords View::framebuffer_origin() const
{
    if (m_origin == ViewOrigin::Center) {
        return 0.5f * m_framebuffer_size;
    } else {
        return {0, 0};
    }
}


void View::set_viewport_scale(float scale)
{
    m_vp_scale = scale;
    rescale_viewport();
}


ViewportCoords View::viewport_center() const
{
    if (m_origin == ViewOrigin::TopLeft) {
        return 0.5f * m_viewport_size;
    } else {
        return {0, 0};
    }
}


ViewportCoords View::viewport_top_left(ViewportCoords offset) const
{
    if (m_origin == ViewOrigin::TopLeft) {
        return offset;
    } else {
        return offset - 0.5f * m_viewport_size;
    }
}


auto View::push_offset(VariCoords offset) -> PopHelper<FramebufferCoords>
{
    m_offset.push_back(this->offset() + to_fb(offset));
    return {m_offset};
}


FramebufferCoords View::offset() const
{
    if (m_offset.empty())
        return {0, 0};
    return m_offset.back();
}


auto View::push_crop(const FramebufferRect& region) -> PopHelper<FramebufferRect>
{
    if (m_crop.empty())
        m_crop.push_back(region.moved(offset()));
    else {
        m_crop.push_back(region.moved(offset()).intersection(m_crop.back()));
    }
    return {m_crop};
}


void View::apply_crop(VkCommandBuffer cmd_buf)
{
    // set scissor region
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = { INT32_MAX, INT32_MAX },
    };
    if (has_crop()) {
        const auto crop = get_crop().moved(framebuffer_origin());
        scissor.offset.x = crop.x.as<int32_t>();
        scissor.offset.y = crop.y.as<int32_t>();
        scissor.extent.width = crop.w.as<uint32_t>();
        scissor.extent.height = crop.h.as<uint32_t>();
    }
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
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


void View::rescale_viewport()
{
    // Decide between vert+/hor+ depending on screen orientation.
    if (m_screen_size.x < m_screen_size.y) {
        // preserve screen width
        float aspect = float(m_screen_size.y.value) / float(m_screen_size.x.value);
        m_viewport_size = {m_vp_scale, m_vp_scale * aspect};
    } else {
        // preserve screen height
        float aspect = float(m_screen_size.x.value) / float(m_screen_size.y.value);
        m_viewport_size = {m_vp_scale * aspect, m_vp_scale};
    }
}


} // namespace xci::graphics
