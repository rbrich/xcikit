// TextTerminal.cpp created on 2018-07-19, part of XCI toolkit
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

#include "TextTerminal.h"
#include <xci/util/string.h>
#include <xci/graphics/Sprites.h>
#include <xci/graphics/Shape.h>
#include <xci/util/log.h>


namespace xci {
namespace widgets {

using graphics::Color;
using text::CodePoint;
using util::Rect_f;
using util::to_utf32;
using namespace util::log;


void TextTerminal::add_string(const std::string& string)
{
    current_line().append(string);
}


void TextTerminal::resize(View& view)
{
    auto& font = theme().font();
    auto pxf = view.framebuffer_ratio();
    font.set_size(unsigned(m_font_size / pxf.y));
    m_cell_size = {(font.max_advance() + 0.5f) * pxf.x,
                   (font.line_height() + 0.5f) * pxf.y};
    m_cells = {int(size().x / m_cell_size.x),
               int(size().y / m_cell_size.y)};
    log_debug("TextTerminal cells {}", m_cells);
}


void TextTerminal::draw(View& view, State state)
{
    auto& font = theme().font();
    auto pxf = view.framebuffer_ratio();
    font.set_size(unsigned(m_font_size / pxf.y));

    graphics::Sprites sprites(font.get_texture(), Color::White());
    graphics::Shape boxes(Color(150, 0, 0));

    Vec2f pen;
    for (CodePoint code_point : to_utf32(current_line().content())) {
        auto glyph = font.get_glyph(code_point);
        if (glyph == nullptr)
            glyph = font.get_glyph(' ');

        Rect_f rect{pen.x + glyph->base_x() * pxf.x,
                    pen.y + (font.ascender() - glyph->base_y()) * pxf.y,
                    glyph->width() * pxf.x,
                    glyph->height() * pxf.y};
        sprites.add_sprite(rect, glyph->tex_coords());

        rect.x = pen.x;
        rect.y = pen.y;
        rect.w = m_cell_size.x;
        rect.h = m_cell_size.y;
        boxes.add_rectangle(rect);

        pen.x += glyph->advance() * pxf.x;
    }

    boxes.draw(view, position());
    sprites.draw(view, position());
}


}} // namespace xci::widgets
