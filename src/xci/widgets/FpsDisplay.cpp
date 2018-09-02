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
#include <xci/config.h>
#include <xci/util/log.h>
#include <xci/util/format.h>
#include <xci/graphics/Renderer.h>

#ifdef XCI_EMBED_SHADERS
#define INCBIN_PREFIX g_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include <incbin/incbin.h>
INCBIN(fps_vert, XCI_SHARE_DIR "/shaders/fps.vert");
INCBIN(fps_frag, XCI_SHARE_DIR "/shaders/fps.frag");
#endif

namespace xci {
namespace widgets {

using namespace xci::graphics;
using namespace xci::util;
using namespace xci::util::log;
using xci::util::format;


FpsDisplay::FpsDisplay()
        : m_quad(Renderer::default_renderer().new_primitives(VertexFormat::V2t2, PrimitiveType::TriFans)),
          m_texture(Renderer::default_renderer().new_texture())
{}


void FpsDisplay::resize(View& view)
{
    float x1 = 0;
    float y1 = 0;
    float x2 = 0.50f;
    float y2 = 0.10f;
    m_quad->clear();
    m_quad->begin_primitive();
    m_quad->add_vertex(x1, y1, 0, 0);
    m_quad->add_vertex(x1, y2, 0, 1);
    m_quad->add_vertex(x2, y2, 1, 1);
    m_quad->add_vertex(x2, y1, 1, 0);
    m_quad->end_primitive();
    m_texture->create({(unsigned)m_fps.resolution(), 1});
    m_text.set_font(theme().font());
}


void FpsDisplay::draw(View& view, State state)
{
    // Measure time from previous frame
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsed = now - m_prevtime;
    m_fps.tick(elapsed.count());
    m_prevtime = now;

    // Draw
    init_shader();
    update_texture();
    m_quad->set_shader(m_shader);
    m_quad->set_texture("u_texture", m_texture);
    m_quad->draw(view, position());

    m_text.set_string(format("{}fps ({:.2f}ms)",
                             m_fps.frame_rate(),
                             m_fps.avg_frame_time() * 1000));
    m_text.resize(view);
    m_text.draw(view, position() + Vec2f{0.02, 0.07f});
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
    bool res = m_shader->load_from_vfs("shaders/fps.vert", "shaders/fps.frag");
#endif
    if (!res) {
        log_error("FPS shader not loaded!");
    }
}


void FpsDisplay::update_texture()
{
    constexpr float sample_max = 1.f / 30.f;
    uint8_t pixels[FpsCounter::max_resolution];
    uint8_t* pixel = pixels;
    m_fps.foreach_sample([&](float sample) {
        *pixel++ = uint8_t(sample / sample_max * 255.f);
    });
    m_texture->update(pixels);
}


}} // namespace xci::widgets
