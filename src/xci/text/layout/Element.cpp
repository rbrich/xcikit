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

#include <xci/graphics/Sprites.h>
#include <xci/graphics/Rectangles.h>

namespace xci {
namespace text {
namespace layout {

using xci::util::Vec2f;
using xci::util::Rect_f;
using xci::graphics::Color;
using xci::graphics::View;


void Word::apply(Page& page)
{
    auto* font = page.style().font();
    if (!font) {
        assert(!"Font is not set!");
        return;
    }

    // Measure word (metrics are affected by string, font, size)
    util::Vec2f advance;
    util::Rect_f bounds;
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


void Word::draw(View& target, const Vec2f& pos) const
{
    auto* font = m_style.font();
    if (!font) {
        assert(!"Font is not set!");
        return;
    }

    auto pxr = target.pixel_ratio();
    font->set_size(unsigned(m_style.size() * pxr.y));

    bool show_bboxes = target.has_debug_flag(View::Debug::GlyphBBox);

    graphics::Sprites sprites(font->get_texture(), m_style.color());
    graphics::Rectangles bboxes(Color(150, 0, 0), Color(250, 50, 50));

    Vec2f pen;
    for (CodePoint code_point : m_string) {
        auto glyph = font->get_glyph(code_point);
        if (glyph == nullptr)
            continue;

        Rect_f rect{pen.x + glyph->base_x() / pxr.x,
                    pen.y - glyph->base_y() / pxr.y,
                    glyph->width() / pxr.x,
                    glyph->height() / pxr.y};
        sprites.add_sprite(rect, glyph->tex_coords());
        if (show_bboxes)
            bboxes.add_rectangle(rect, 2 / pxr.x);

        pen.x += glyph->advance() / pxr.x;
    }

    if (show_bboxes)
        bboxes.draw(target, pos + m_pos);
    sprites.draw(target, pos + m_pos);
}


}}} // namespace xci::text::layout
