// GlyphCluster.cpp created on 2019-12-16 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "GlyphCluster.h"
#include "Font.h"
#include <xci/graphics/Renderer.h>
#include <xci/core/string.h>

namespace xci::text {

using namespace xci::graphics;
using namespace xci::core;


GlyphCluster::GlyphCluster(graphics::Renderer& renderer, Font& font)
    : m_font(font), m_sprites(renderer, font.texture(), font.sampler())
{}


FramebufferSize GlyphCluster::add_glyph(const graphics::View& view, GlyphIndex glyph_index)
{
    auto* glyph = m_font.get_glyph(glyph_index);
    if (glyph == nullptr)
        return {};

    auto bearing = FramebufferSize{glyph->bearing()};
    auto glyph_size = FramebufferSize{glyph->size()};
    const FramebufferRect rect{
            m_pen.x + bearing.x,
            m_pen.y - bearing.y,
            glyph_size.x,
            glyph_size.y};
    m_sprites.add_sprite(rect, glyph->tex_coords());

    return FramebufferSize{glyph->advance()};
}


void GlyphCluster::add_string(const graphics::View& view, const std::string& str)
{
    for (const auto& shaped_glyph : m_font.shape_text(str)) {
        add_glyph(view, shaped_glyph.glyph_index);
        m_pen += FramebufferCoords{shaped_glyph.advance};
    }
}


} // namespace xci::text
