// GlyphCluster.cpp created on 2019-12-16 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "GlyphCluster.h"
#include "Font.h"
#include <xci/graphics/Renderer.h>
#include <xci/core/string.h>

namespace xci::text {

using namespace xci::graphics;
using namespace xci::core;


GlyphCluster::GlyphCluster(graphics::Renderer& renderer, Font& font)
    : m_font(font), m_sprites(renderer, font.texture())
{}


void GlyphCluster::add_glyph(const graphics::View& view, CodePoint code_point)
{
    auto glyph = m_font.get_glyph(code_point);
    if (glyph == nullptr)
        return;

    auto bearing = view.size_to_viewport(FramebufferSize{glyph->bearing()});
    auto glyph_size = view.size_to_viewport(FramebufferSize{glyph->size()});
    ViewportRect rect{m_pen.x + bearing.x,
                      m_pen.y - bearing.y,
                      glyph_size.x,
                      glyph_size.y};
    m_sprites.add_sprite(rect, glyph->tex_coords());

    m_pen.x += view.size_to_viewport(FramebufferPixels{glyph->advance()});
}


void GlyphCluster::add_string(const graphics::View& view, const std::string& str)
{
    for (CodePoint code_point : to_utf32(str)) {
        add_glyph(view, code_point);
    }
}


} // namespace xci::text
