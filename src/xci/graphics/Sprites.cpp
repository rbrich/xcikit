// Sprites.cpp created on 2018-03-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Sprites.h"

#include <xci/config.h>

namespace xci::graphics {


Sprites::Sprites(Renderer& renderer, Texture& texture, Sampler& sampler, Color color)
        : m_texture(texture), m_sampler(sampler), m_color(color),
          m_quads(renderer, VertexFormat::V2t2, PrimitiveType::TriFans),
          m_shader(renderer.get_shader("sprite",
                      texture.color_format() == ColorFormat::LinearGrey ? "sprite_r" : "sprite"))
{}


void Sprites::reserve(size_t num)
{
    m_quads.reserve(4 * num);
}


void Sprites::add_sprite(const FramebufferRect& rect)
{
    auto ts = m_texture.size();
    add_sprite(rect, {0, 0, ts.x, ts.y});
}


// Position a sprite with cutoff from the texture
void Sprites::add_sprite(const FramebufferRect& rect, const Rect_u& texrect)
{
    const auto x1 = rect.x;
    const auto y1 = rect.y;
    const auto x2 = rect.x + rect.w;
    const auto y2 = rect.y + rect.h;
    const auto ts = m_texture.size();
    const float tl = (float)texrect.left() / ts.x;
    const float tr = (float)texrect.right() / ts.x;
    const float tb = (float)texrect.bottom() / ts.y;
    const float tt = (float)texrect.top() / ts.y;

    m_quads.begin_primitive();
    m_quads.add_vertex({x1, y1}).uv(tl, tt);
    m_quads.add_vertex({x1, y2}).uv(tl, tb);
    m_quads.add_vertex({x2, y2}).uv(tr, tb);
    m_quads.add_vertex({x2, y1}).uv(tr, tt);
    m_quads.end_primitive();
}


void Sprites::update()
{
    m_quads.clear_uniforms();
    m_quads.set_uniform(1, m_color);
    m_quads.set_blend(BlendFunc::AlphaBlend);
    m_quads.set_texture(2, m_texture, m_sampler);
    m_quads.set_shader(m_shader);
    m_quads.update();
}


void Sprites::draw(View& view, VariCoords pos)
{
    m_quads.draw(view, pos);
}


// -----------------------------------------------------------------------------


ColoredSprites::ColoredSprites(Renderer& renderer,
                               Texture& texture, Sampler& sampler, Color color)
    : m_texture(texture), m_sampler(sampler), m_color(color),
      m_quads(renderer, VertexFormat::V2c4t2, PrimitiveType::TriFans),
      m_shader(renderer.get_shader("sprite_c", "sprite_c"))
{}


void ColoredSprites::reserve(size_t num)
{
    m_quads.reserve(num * 4);
}


void ColoredSprites::add_sprite(const FramebufferRect& rect)
{
    auto ts = m_texture.size();
    add_sprite(rect, {0, 0, ts.x, ts.y});
}


// Position a sprite with cutoff from the texture
void ColoredSprites::add_sprite(const FramebufferRect& rect, const Rect_u& texrect)
{
    const auto x1 = rect.x;
    const auto y1 = rect.y;
    const auto x2 = rect.x + rect.w;
    const auto y2 = rect.y + rect.h;
    const auto ts = m_texture.size();
    const float tl = (float)texrect.left() / (float)ts.x;
    const float tr = (float)texrect.right() / (float)ts.x;
    const float tb = (float)texrect.bottom() / (float)ts.y;
    const float tt = (float)texrect.top() / (float)ts.y;

    m_quads.begin_primitive();
    m_quads.add_vertex({x1, y1}).color(m_color).uv(tl, tt);
    m_quads.add_vertex({x1, y2}).color(m_color).uv(tl, tb);
    m_quads.add_vertex({x2, y2}).color(m_color).uv(tr, tb);
    m_quads.add_vertex({x2, y1}).color(m_color).uv(tr, tt);
    m_quads.end_primitive();
}


void ColoredSprites::update()
{
    m_quads.set_texture(1, m_texture, m_sampler);
    m_quads.set_shader(m_shader);
    m_quads.set_blend(BlendFunc::AlphaBlend);
    m_quads.update();
}


void ColoredSprites::draw(View& view, VariCoords pos)
{
    m_quads.draw(view, pos);
}


} // namespace xci::graphics
