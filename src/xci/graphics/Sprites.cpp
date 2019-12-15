// Sprites.cpp created on 2018-03-14, part of XCI toolkit
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

#include "Sprites.h"

#include <xci/config.h>
#include <xci/core/log.h>

namespace xci::graphics {

using namespace xci::core::log;


Sprites::Sprites(Renderer& renderer, Texture& texture, const Color& color)
        : m_texture(texture), m_color(color),
          m_quads(renderer, VertexFormat::V2t2, PrimitiveType::TriFans),
          m_shader(renderer.get_shader(ShaderId::Sprite))
{}


void Sprites::reserve(size_t num)
{
    m_quads.reserve(4 * num);
}


void Sprites::add_sprite(const ViewportRect& rect)
{
    auto ts = m_texture.size();
    add_sprite(rect, {0, 0, ts.x, ts.y});
}


// Position a sprite with cutoff from the texture
void Sprites::add_sprite(const ViewportRect& rect, const Rect_u& texrect)
{
    auto x1 = rect.x;
    auto y1 = rect.y;
    auto x2 = rect.x + rect.w;
    auto y2 = rect.y + rect.h;
    auto ts = m_texture.size();
    float tl = (float)texrect.left() / ts.x;
    float tr = (float)texrect.right() / ts.x;
    float tb = (float)texrect.bottom() / ts.y;
    float tt = (float)texrect.top() / ts.y;

    m_quads.begin_primitive();
    m_quads.add_vertex({x1, y1}, tl, tt);
    m_quads.add_vertex({x1, y2}, tl, tb);
    m_quads.add_vertex({x2, y2}, tr, tb);
    m_quads.add_vertex({x2, y1}, tr, tt);
    m_quads.end_primitive();
}


void Sprites::update()
{
    m_quads.add_uniform(1, m_color);
    m_quads.set_blend(BlendFunc::AlphaBlend);
    m_quads.set_texture(2, m_texture);
    m_quads.set_shader(m_shader);
    m_quads.update();
}


void Sprites::draw(View& view, const ViewportCoords& pos)
{
    m_quads.draw(view, pos);
}


// -----------------------------------------------------------------------------


ColoredSprites::ColoredSprites(Renderer& renderer,
                               Texture& texture, const Color& color)
    : m_texture(texture), m_color(color),
      m_quads(renderer, VertexFormat::V2c4t2, PrimitiveType::TriFans),
      m_shader(renderer.get_shader(ShaderId::SpriteC))
{}


void ColoredSprites::reserve(size_t num)
{
    m_quads.reserve(num * 4);
}


void ColoredSprites::add_sprite(const ViewportRect& rect)
{
    auto ts = m_texture.size();
    add_sprite(rect, {0, 0, ts.x, ts.y});
}


// Position a sprite with cutoff from the texture
void ColoredSprites::add_sprite(const ViewportRect& rect, const Rect_u& texrect)
{
    auto x1 = rect.x;
    auto y1 = rect.y;
    auto x2 = rect.x + rect.w;
    auto y2 = rect.y + rect.h;
    auto ts = m_texture.size();
    float tl = (float)texrect.left() / ts.x;
    float tr = (float)texrect.right() / ts.x;
    float tb = (float)texrect.bottom() / ts.y;
    float tt = (float)texrect.top() / ts.y;

    m_quads.begin_primitive();
    m_quads.add_vertex({x1, y1}, m_color, tl, tt);
    m_quads.add_vertex({x1, y2}, m_color, tl, tb);
    m_quads.add_vertex({x2, y2}, m_color, tr, tb);
    m_quads.add_vertex({x2, y1}, m_color, tr, tt);
    m_quads.end_primitive();
}


void ColoredSprites::update()
{
    m_quads.set_texture(1, m_texture);
    m_quads.set_shader(m_shader);
    m_quads.set_blend(BlendFunc::AlphaBlend);
    m_quads.update();
}


void ColoredSprites::draw(View& view, const ViewportCoords& pos)
{
    m_quads.draw(view, pos);
}


} // namespace xci::graphics
