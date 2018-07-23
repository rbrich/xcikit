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


// Basic palette of 4-bit colors
static constexpr Color c_colors_4bit[16] = {
    {0,0,0},        // 0 - black
    {178,23,23},    // 1 - red
    {23,178,23},    // 2 - green
    {178,103,23},   // 3 - yellow
    {23,23,178},    // 4 - blue
    {178,23,178},   // 5 - magenta
    {23,178,178},   // 6 - cyan
    {178,178,178},  // 7 - white
    {104,104,104},  // 8 - bright black
    {255,84,84},    // 9 - bright red
    {84,255,84},    // 10 - bright green
    {255,255,84},   // 11 - bright yellow
    {84,84,255},    // 12 - bright blue
    {255,84,255},   // 13 - bright magenta
    {84,255,255},   // 14 - bright cyan
    {255,255,255},  // 15 - bright white
};


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


void TextTerminal::set_color(Color4bit fg, Color4bit bg)
{
    uint8_t encoded_color = (uint8_t(bg) << 4) + uint8_t(fg);
    uint8_t seq[] = {0xc1, encoded_color};
    current_line().append(std::string(std::begin(seq), std::end(seq)));
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

    graphics::ColoredSprites sprites(font.get_texture(), c_colors_4bit[7]);
    graphics::Shape boxes(c_colors_4bit[0]);

    Vec2f pen;
    auto buffer_last = std::min(int(m_buffer.size()), m_buffer_offset + m_cells.y);
    for (int line_idx = m_buffer_offset; line_idx < buffer_last; line_idx++) {
        auto& line = m_buffer[line_idx].content();
        for (auto pos = line.cbegin(); pos != line.cend(); ) {
            if (*pos == '\n') {
                // newline is ignored and finishes line processing
                break;
            }
            if (*pos == '\xc1') {
                // decode 4-bit colors
                ++pos;
                auto& fg = c_colors_4bit[*pos & 0x0f];
                auto& bg = c_colors_4bit[*pos >> 4];
                sprites.set_color(fg);
                boxes.set_fill_color(bg);
                ++pos;
                continue;
            }

            // extract single UTF-8 character
            auto end_pos = utf8_next(pos);
            auto ch = line.substr(pos - line.cbegin(), end_pos - pos);
            pos = end_pos;

            CodePoint code_point = to_utf32(ch)[0];
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
