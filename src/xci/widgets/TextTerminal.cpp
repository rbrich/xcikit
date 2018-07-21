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
using namespace util;
using namespace util::log;


int terminal::Line::length() const
{
    return utf8_length(m_content);
}


void TextTerminal::add_text(const std::string& text)
{
    // Add characters up to terminal width (columns)
    int length = current_line().length();
    for (auto pos = text.cbegin(); pos != text.cend(); ) {
        // Check for newline character
        bool new_line = false;
        if (*pos == '\n') {
            current_line().append("\n");
            new_line = true;
        }

        // Check line length
        if (length >= m_cells.x || new_line) {
            // Add new line
            m_buffer.add_line();
            ++m_cursor.y;
            m_cursor.x = 0;
            length = 0;
        }

        // Add character to current line
        auto end_pos = utf8_next(pos);
        if (!new_line)
            current_line().append(text.substr(pos - text.cbegin(), end_pos - pos));
        pos = end_pos;
        ++length;
    }
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
    auto buffer_last = std::min(int(m_buffer.size()), m_buffer_offset + m_cells.y);
    for (int line_idx = m_buffer_offset; line_idx < buffer_last; line_idx++) {
        for (CodePoint code_point : to_utf32(m_buffer[line_idx].content())) {
            if (code_point == '\n')
                break;
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
        pen.x = 0;
        pen.y += font.line_height() * pxf.y;
    }

    boxes.draw(view, position());
    sprites.draw(view, position());
}


}} // namespace xci::widgets
