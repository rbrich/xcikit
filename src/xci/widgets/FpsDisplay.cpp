// FpsDisplay.cpp created on 2018-04-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "FpsDisplay.h"
#include <xci/graphics/Renderer.h>
#include <fmt/format.h>
#include <chrono>

namespace xci::widgets {

using namespace xci::graphics;
using namespace xci::graphics::unit_literals;
using namespace xci::core;
using fmt::format;
using namespace std::chrono_literals;


FpsDisplay::FpsDisplay(Theme& theme)
        : Widget(theme),
          m_quad(theme.renderer(), VertexFormat::V2t2, PrimitiveType::TriFans),
          m_shader(theme.renderer().get_shader("fps", "fps")),
          m_texture(theme.renderer())
{
    m_texture.create({(unsigned)FpsCounter::resolution, 1}, {ColorFormat::LinearGrey});

    // default size in "scalable" units
    set_size({25_vp, 5_vp});
}


void FpsDisplay::resize(View& view)
{
    Widget::resize(view);
    view.finish_draw();

    m_quad.clear();
    create_sprite();
    update_texture();

    m_text.set_font(theme().base_font());
    m_text.set_font_size(size().y / 2);
}


void FpsDisplay::update(View& view, State state)
{
    m_since_last_update += state.elapsed;
    if (!m_frozen && m_since_last_update >= 50ms) {  // update 20 times / second
        m_since_last_update = {};
        update_texture();
        m_text.set_string(format("{}fps ({:.2f}ms)",
                        m_fps.frame_rate(),
                        m_fps.avg_frame_time() * 1000));
        m_text.update(view);
        m_quad.update();
    }

    if (state.elapsed > 400ms && !m_frozen) {
        // Almost 1 seconds since last refresh - freeze the counter
        m_frozen = true;
        view.refresh();
        return;
    }

    // Still fast enough, reset timeout
    view.window()->set_refresh_timeout(500ms, false);
    m_frozen = false;
}


void FpsDisplay::draw(View& view)
{
    // Measure time from previous frame
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsed = now - m_prev_time;
    m_fps.tick(elapsed.count());
    m_prev_time = now;

    if (m_frozen)
        return; // don't draw anything

    // Draw
    m_quad.draw(view, position());

    auto font_size = size().y / 2;
    auto offset = size().y / 5;
    m_text.draw(view, position() + FramebufferCoords{offset, offset + font_size});
}


void FpsDisplay::create_sprite()
{
    auto x1 = 0_fb;
    auto y1 = 0_fb;
    auto x2 = size().x;
    auto y2 = size().y;
    m_quad.reserve(4);
    m_quad.begin_primitive();
    m_quad.add_vertex({x1, y1}).uv(0, 1);
    m_quad.add_vertex({x1, y2}).uv(0, 0);
    m_quad.add_vertex({x2, y2}).uv(1, 0);
    m_quad.add_vertex({x2, y1}).uv(1, 1);
    m_quad.end_primitive();
    m_quad.set_texture(1, m_texture);
    m_quad.set_shader(m_shader);
    m_quad.update();
}


void FpsDisplay::update_texture()
{
    constexpr float sample_max = 1.f / 30.f;
    uint8_t pixels[FpsCounter::resolution];
    uint8_t* pixel = pixels;
    m_fps.foreach_sample([&](float sample) {
        *pixel++ = uint8_t(sample / sample_max * 255.f);
    });
    m_texture.write(pixels);
    m_texture.update();
}


} // namespace xci::widgets
