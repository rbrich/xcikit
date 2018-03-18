// Element.cpp created on 2018-03-18, part of XCI toolkit
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

#include "Element.h"

#include "../../graphics/Sprites.h"

namespace xci {
namespace text {
namespace layout {


void Word::apply(Page& page)
{
    // Measure word (metrics are affected by string, font, size)
    util::Vec2f advance;
    util::Rect_f bounds;
    auto* font = page.style().font();
    auto pxr = page.target_pixel_ratio();
    font->set_size(unsigned(m_style.size() * pxr.y));
    for (CodePoint code_point : m_string) {
        auto glyph = font->get_glyph(code_point);

        // Expand text bounds by glyph bounds
        util::Rect_f m;
        m.x = advance.x + glyph->base_x() / pxr.x;
        m.y = 0.0f - glyph->base_y() / pxr.y;
        m.w = glyph->width() / pxr.x;
        m.h = glyph->height() / pxr.y;  // ft_to_float(gm.height)
        bounds.extend(m);

        advance.x += glyph->advance() / pxr.x;
    }

    // Check line end
    if (page.width() > 0.0 && page.pen().x + advance.x > page.width()) {
        page.finish_line();
    }

    // Set position according to pen
    m_pos = page.pen();
    bounds.x += m_pos.x;
    bounds.y += m_pos.y;

    page.set_element_bounds(bounds);
    page.advance_pen(advance);

    m_style = page.style();
}


void Word::draw(graphics::View& target, const util::Vec2f& pos) const
{
    auto* font = m_style.font();
    if (!font)
        return;
    auto pxr = target.pixel_ratio();
    font->set_size(unsigned(m_style.size() * pxr.y));

    graphics::Sprites sprites(font->get_texture());

    Vec2f pen;
    for (CodePoint code_point : m_string) {
        auto glyph = font->get_glyph(code_point);
        if (glyph == nullptr)
            continue;

        sprites.add_sprite({pen.x + glyph->base_x() / pxr.x,
                            pen.y - glyph->base_y() / pxr.y,
                            glyph->width() / pxr.x,
                            glyph->height() / pxr.y},
                           glyph->tex_coords(),
                           m_style.color());

#if 0
        sf::RectangleShape bbox;
        sf::FloatRect m;
        m.left = ft_to_float(gm.horiBearingX) + pen;
        m.top = -ft_to_float(gm.horiBearingY);
        m.width = ft_to_float(gm.width);
        m.height = ft_to_float(gm.height);
        bbox.setPosition(m.left, m.top);
        bbox.setSize({m.width, m.height});
        bbox.setFillColor(sf::Color::Transparent);
        bbox.setOutlineColor(sf::Color::Blue);
        bbox.setOutlineThickness(1);
        target.draw(bbox, states);
#endif

        pen.x += glyph->advance() / pxr.x;
    }

    sprites.draw(target, pos + m_pos);
}


}}} // namespace xci::text::layout
