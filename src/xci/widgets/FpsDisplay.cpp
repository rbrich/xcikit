// FpsDisplay.cpp created on 2018-04-14, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
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

#include "FpsDisplay.h"
#include <xci/core/format.h>
#include <xci/graphics/Renderer.h>
#include <chrono>

namespace xci::widgets {

using namespace xci::graphics;
using namespace xci::graphics::unit_literals;
using namespace xci::core;
using namespace xci::core::log;
using xci::core::format;
using namespace std::chrono_literals;


FpsDisplay::FpsDisplay(Theme& theme)
        : Widget(theme),
          m_quad(theme.renderer(), VertexFormat::V2t2, PrimitiveType::TriFans),
          m_shader(theme.renderer().get_shader(ShaderId::Fps)),
          m_texture(theme.renderer())
{
    m_texture.create({(unsigned)m_fps.resolution(), 1});

    // default size in "scalable" units
    set_size({0.50f, 0.10f});
    create_sprite();
}


void FpsDisplay::resize(View& view)
{
    m_quad.clear();
    create_sprite();

    m_text.set_font(theme().font());
    m_text.set_font_size(size().y / 2);
}


void FpsDisplay::update(View& view, State state)
{
    if (!m_frozen) {
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
    std::chrono::duration<float> elapsed = now - m_prevtime;
    m_fps.tick(elapsed.count());
    m_prevtime = now;

    if (m_frozen)
        return; // don't draw anything

    // Draw
    m_quad.draw(view, position());

    auto font_size = size().y / 2;
    auto offset = size().y / 5;
    m_text.draw(view, position() + ViewportCoords{offset, offset + font_size});
}


void FpsDisplay::create_sprite()
{
    auto x1 = 0_vp;
    auto y1 = 0_vp;
    auto x2 = size().x;
    auto y2 = size().y;
    m_quad.reserve(4);
    m_quad.begin_primitive();
    m_quad.add_vertex({x1, y1}, 0, 0);
    m_quad.add_vertex({x1, y2}, 0, 1);
    m_quad.add_vertex({x2, y2}, 1, 1);
    m_quad.add_vertex({x2, y1}, 1, 0);
    m_quad.end_primitive();
    m_quad.set_texture(1, m_texture);
    m_quad.set_shader(m_shader);
    m_quad.update();
}


void FpsDisplay::update_texture()
{
    constexpr float sample_max = 1.f / 30.f;
    uint8_t pixels[FpsCounter::max_resolution];
    uint8_t* pixel = pixels;
    m_fps.foreach_sample([&](float sample) {
        *pixel++ = uint8_t(sample / sample_max * 255.f);
    });
    m_texture.write(pixels);
    m_texture.update();
}


} // namespace xci::widgets
