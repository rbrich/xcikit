// Sprites.cpp created on 2018-03-14, part of XCI toolkit
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

#include "Sprites.h"

#include <xci/util/log.h>

#ifdef XCI_EMBED_SHADERS
#define INCBIN_PREFIX g_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "incbin.h"
INCBIN(sprite_vert, XCI_SHARE_DIR "/shaders/sprite.vert");
INCBIN(sprite_frag, XCI_SHARE_DIR "/shaders/sprite.frag");
#endif

namespace xci {
namespace graphics {

using namespace xci::util::log;


Sprites::Sprites(TexturePtr& texture, const Color& color)
        : m_texture(texture), m_color(color),
          m_sprites(Primitives::VertexFormat::V2T2)
{}


void Sprites::add_sprite(const Rect_f& rect)
{
    auto ts = m_texture->size();
    add_sprite(rect, {0, 0, ts.x, ts.y});
}


// Position a sprite with cutoff from the texture
void Sprites::add_sprite(const Rect_f& rect, const Rect_u& texrect)
{
    float x1 = rect.x;
    float y1 = -rect.y;
    float x2 = rect.x + rect.w;
    float y2 = -rect.y - rect.h;
    auto ts = m_texture->size();
    float tl = (float)texrect.left() / ts.x;
    float tr = (float)texrect.right() / ts.x;
    float tb = (float)texrect.bottom() / ts.y;
    float tt = (float)texrect.top() / ts.y;

    m_sprites.begin_primitive();
    m_sprites.add_vertex(x2, y1, tr, tt);
    m_sprites.add_vertex(x2, y2, tr, tb);
    m_sprites.add_vertex(x1, y2, tl, tb);
    m_sprites.add_vertex(x1, y1, tl, tt);
    m_sprites.end_primitive();
}


void Sprites::draw(View& view, const Vec2f& pos)
{
    init_shader();
    m_sprites.set_shader(m_shader);
    m_sprites.set_uniform("u_color",
                          m_color.red_f(), m_color.green_f(),
                          m_color.blue_f(), m_color.alpha_f());
    m_sprites.set_texture("u_texture", m_texture);
    m_sprites.draw(view, pos);
}


void Sprites::init_shader()
{
    if (m_shader)
        return;
    auto& renderer = Renderer::default_renderer();
    m_shader = renderer.new_shader(Renderer::ShaderId::Sprite);

#ifdef XCI_EMBED_SHADERS
    bool res = m_shader->load_from_memory(
                (const char*)g_sprite_vert_data, g_sprite_vert_size,
                (const char*)g_sprite_frag_data, g_sprite_frag_size);
)
#else
    bool res = m_shader->load_from_file("shaders/sprite.vert",
                                        "shaders/sprite.frag");
#endif
    if (!res) {
        log_error("Rectangle shader not loaded!");
    }
}


}} // namespace xci::graphics
