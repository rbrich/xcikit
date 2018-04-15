// FpsDisplay.cpp created on 2018-04-14, part of XCI toolkit
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

#include "FpsDisplay.h"
#include <xci/util/log.h>

#ifdef XCI_EMBED_SHADERS
#define INCBIN_PREFIX g_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "incbin.h"
INCBIN(fps_vert, XCI_SHARE_DIR "/shaders/fps.vert");
INCBIN(fps_frag, XCI_SHARE_DIR "/shaders/fps.frag");
#endif

namespace xci {
namespace widgets {

using namespace xci::graphics;
using namespace xci::util::log;


FpsDisplay::FpsDisplay(const util::FpsCounter& fps_counter, Theme& theme)
        : m_fps_counter(fps_counter), m_theme(theme),
          m_quad(Renderer::default_renderer().new_primitives(VertexFormat::V2t2, PrimitiveType::TriFans)),
          m_texture(Renderer::default_renderer().new_texture())
{}


void FpsDisplay::resize(const graphics::View& target)
{
    float x1 = 0;
    float y1 = -0.10f;
    float x2 = 0.30;
    float y2 = 0.0;
    m_quad->clear();
    m_quad->begin_primitive();
    m_quad->add_vertex(x2, y1, 1, 0);
    m_quad->add_vertex(x2, y2, 1, 1);
    m_quad->add_vertex(x1, y2, 0, 1);
    m_quad->add_vertex(x1, y1, 0, 0);
    m_quad->end_primitive();
    m_texture->create({(unsigned)m_fps_counter.ticks().size(), 1});
    m_text.set_font(m_theme.font());
}


void FpsDisplay::draw(graphics::View& view, const util::Vec2f& pos)
{
    init_shader();
    update_texture();
    m_quad->set_shader(m_shader);
    m_quad->set_texture("u_texture", m_texture);
    m_quad->draw(view, pos);

    m_text.set_string(std::to_string(m_fps_counter.fps()) + " fps");
    m_text.draw(view, pos + Vec2f{0.02, 0.07f});
}


void FpsDisplay::init_shader()
{
    if (m_shader)
        return;
    auto& renderer = Renderer::default_renderer();
    m_shader = renderer.new_shader(ShaderId::Custom);

#ifdef XCI_EMBED_SHADERS
    bool res = m_shader->load_from_memory(
                (const char*)g_fps_vert_data, g_fps_vert_size,
                (const char*)g_fps_frag_data, g_fps_frag_size);
)
#else
    bool res = m_shader->load_from_file(
            "shaders/fps.vert", "shaders/fps.frag");
#endif
    if (!res) {
        log_error("FPS shader not loaded!");
    }
}


void FpsDisplay::update_texture()
{
    auto& ticks = m_fps_counter.ticks();
    auto tick_max = std::max_element(ticks.begin(), ticks.end());
    uint8_t pixels[ticks.size()];
    uint8_t* pixel = pixels + ticks.size() - m_fps_counter.current_tick() - 1;
    for (auto t : ticks) {
        if (pixel >= pixels + ticks.size())
            pixel = pixels;
        *pixel++ = uint8_t(float(t) / *tick_max * 255.f);
    }
    m_texture->update(pixels);
}


}} // namespace xci::widgets
