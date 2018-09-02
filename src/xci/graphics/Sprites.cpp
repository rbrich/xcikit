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

#include <xci/config.h>
#include <xci/util/log.h>

#ifdef XCI_EMBED_SHADERS
#define INCBIN_PREFIX g_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include <incbin/incbin.h>
INCBIN(sprite_vert, XCI_SHARE_DIR "/shaders/sprite.vert");
INCBIN(sprite_frag, XCI_SHARE_DIR "/shaders/sprite.frag");
INCBIN(sprite_c_vert, XCI_SHARE_DIR "/shaders/sprite_c.vert");
INCBIN(sprite_c_frag, XCI_SHARE_DIR "/shaders/sprite_c.frag");
#endif

namespace xci {
namespace graphics {

using namespace xci::util::log;


Sprites::Sprites(TexturePtr& texture, const Color& color,
                 Renderer& renderer)
        : m_texture(texture), m_color(color),
          m_trifans(renderer.create_primitives(VertexFormat::V2t2,
                                               PrimitiveType::TriFans))
{}


void Sprites::reserve(size_t num)
{
    m_trifans->reserve(num, 4 * num);
}


void Sprites::add_sprite(const Rect_f& rect)
{
    auto ts = m_texture->size();
    add_sprite(rect, {0, 0, ts.x, ts.y});
}


// Position a sprite with cutoff from the texture
void Sprites::add_sprite(const Rect_f& rect, const Rect_u& texrect)
{
    float x1 = rect.x;
    float y1 = rect.y;
    float x2 = rect.x + rect.w;
    float y2 = rect.y + rect.h;
    auto ts = m_texture->size();
    float tl = (float)texrect.left() / ts.x;
    float tr = (float)texrect.right() / ts.x;
    float tb = (float)texrect.bottom() / ts.y;
    float tt = (float)texrect.top() / ts.y;

    m_trifans->begin_primitive();
    m_trifans->add_vertex(x1, y1, tl, tt);
    m_trifans->add_vertex(x1, y2, tl, tb);
    m_trifans->add_vertex(x2, y2, tr, tb);
    m_trifans->add_vertex(x2, y1, tr, tt);
    m_trifans->end_primitive();
}


void Sprites::draw(View& view, const Vec2f& pos)
{
    init_shader();
    m_trifans->set_shader(m_shader);
    m_trifans->set_uniform("u_color",
                          m_color.red_f(), m_color.green_f(),
                          m_color.blue_f(), m_color.alpha_f());
    m_trifans->set_texture("u_texture", m_texture);
    m_trifans->set_blend(Primitives::BlendFunc::AlphaBlend);
    m_trifans->draw(view, pos);
}


void Sprites::init_shader()
{
    if (m_shader)
        return;
    auto& renderer = Renderer::default_renderer();
    m_shader = renderer.get_or_create_shader(ShaderId::Sprite);
    if (m_shader->is_ready())
        return;

#ifdef XCI_EMBED_SHADERS
    bool res = m_shader->load_from_memory(
                (const char*)g_sprite_vert_data, g_sprite_vert_size,
                (const char*)g_sprite_frag_data, g_sprite_frag_size);
)
#else
    bool res = m_shader->load_from_vfs("shaders/sprite.vert",
                                       "shaders/sprite.frag");
#endif
    if (!res) {
        log_error("Rectangle shader not loaded!");
    }
}


ColoredSprites::ColoredSprites(TexturePtr& texture, const Color& color,
                               Renderer& renderer)
    : m_texture(texture), m_color(color),
      m_trifans(renderer.create_primitives(VertexFormat::V2c4t2,
                                           PrimitiveType::TriFans))
{}


void ColoredSprites::reserve(size_t num)
{
    m_trifans->reserve(num, 4);
}


void ColoredSprites::add_sprite(const Rect_f& rect)
{
    auto ts = m_texture->size();
    add_sprite(rect, {0, 0, ts.x, ts.y});
}


// Position a sprite with cutoff from the texture
void ColoredSprites::add_sprite(const Rect_f& rect, const Rect_u& texrect)
{
    float x1 = rect.x;
    float y1 = rect.y;
    float x2 = rect.x + rect.w;
    float y2 = rect.y + rect.h;
    auto ts = m_texture->size();
    float tl = (float)texrect.left() / ts.x;
    float tr = (float)texrect.right() / ts.x;
    float tb = (float)texrect.bottom() / ts.y;
    float tt = (float)texrect.top() / ts.y;

    m_trifans->begin_primitive();
    m_trifans->add_vertex(x1, y1, m_color, tl, tt);
    m_trifans->add_vertex(x1, y2, m_color, tl, tb);
    m_trifans->add_vertex(x2, y2, m_color, tr, tb);
    m_trifans->add_vertex(x2, y1, m_color, tr, tt);
    m_trifans->end_primitive();
}


void ColoredSprites::draw(View& view, const Vec2f& pos)
{
    init_shader();
    m_trifans->set_shader(m_shader);
    m_trifans->set_texture("u_texture", m_texture);
    m_trifans->set_blend(Primitives::BlendFunc::AlphaBlend);
    m_trifans->draw(view, pos);
}


void ColoredSprites::init_shader()
{
    if (m_shader)
        return;
    auto& renderer = Renderer::default_renderer();
    m_shader = renderer.get_or_create_shader(ShaderId::SpriteC);

#ifdef XCI_EMBED_SHADERS
    bool res = m_shader->load_from_memory(
                (const char*)g_sprite_vert_c_data, g_sprite_vert_c_size,
                (const char*)g_sprite_frag_c_data, g_sprite_frag_c_size);
)
#else
    bool res = m_shader->load_from_vfs("shaders/sprite_c.vert",
                                       "shaders/sprite_c.frag");
#endif
    if (!res) {
        log_error("Rectangle shader not loaded!");
    }
}


}} // namespace xci::graphics
