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
using std::min;
using namespace std::chrono;
using namespace std::chrono_literals;


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


static constexpr uint8_t c_font_style_mask = 0b00000011;
static constexpr uint8_t c_decoration_mask = 0b00011100;
static constexpr uint8_t c_mode_mask       = 0b01100000;
static constexpr int c_decoration_shift = 2;
static constexpr int c_mode_shift = 5;


void terminal::Line::insert(int pos, std::string_view string)
{
    int current = 0;
    for (auto it = m_content.cbegin(); it != m_content.cend(); it = utf8_next(it)) {
        if (*it == '\xc0' || *it == '\xc1')
            continue;
        if (pos == current) {
            m_content.insert(it, string.cbegin(), string.cend());
            return;
        }
        ++current;
    }
    assert(pos >= current);
    m_content.append(string.cbegin(), string.cend());
}


void terminal::Line::replace(int pos, std::string_view string)
{
    int current = 0;
    for (auto it = m_content.cbegin(); it != m_content.cend(); it = utf8_next(it)) {
        if (*it == '\xc0' || *it == '\xc1')
            continue;
        if (pos == current) {
            auto end = it;
            for (size_t i = 0; i < string.size(); i++) {
                end = utf8_next(end);
            }
            m_content.replace(it, end, string.cbegin(), string.cend());
            return;
        }
        ++current;
    }
    assert(pos >= current);
    m_content.append(string.cbegin(), string.cend());
}


void terminal::Line::erase(int first, int num)
{
    int current = 0;
    std::string::const_iterator erase_start = m_content.cend();
    std::string::const_iterator erase_end = m_content.cend();
    for (auto pos = m_content.cbegin(); pos != m_content.cend(); pos = utf8_next(pos)) {
        if (*pos == '\xc0' || *pos == '\xc1')
            continue;
        if (current == first) {
            erase_start = pos;
        }
        if (current > first) {
            if (num > 0)
                --num;
            else {
                erase_end = pos;
            }
        }
        ++current;
    }
    if (erase_end != m_content.cend())
        m_content.erase(erase_start - m_content.cbegin(), erase_end - erase_start);
    else
        m_content.erase(erase_start - m_content.cbegin());
}


int terminal::Line::length() const
{
    int length = 0;
    for (auto pos = m_content.cbegin(); pos != m_content.cend(); pos = utf8_next(pos)) {
        if (*pos != '\xc0' && *pos != '\xc1' && *pos != '\n')
            ++length;
    }
    return length;
}


void TextTerminal::add_text(const std::string& text)
{
    // Add characters up to terminal width (columns)
    for (auto it = text.cbegin(); it != text.cend(); ) {
        // Special handling for newline character
        bool new_line = false;
        if (*it == '\n') {
            ++it;
            new_line = true;
        }

        // Check line length
        if (m_cursor.x >= m_cells.x) {
            new_line =  true;
        }

        if (new_line) {
            // Add new line
            m_buffer.add_line();
            ++m_cursor.y;
            m_cursor.x = 0;
            continue;
        }

        // Add character to current line
        auto end_pos = utf8_next(it);
        current_line().replace(m_cursor.x, text.substr(it - text.cbegin(), end_pos - it));
        it = end_pos;
        ++m_cursor.x;
    }
}


void TextTerminal::set_color(Color4bit fg, Color4bit bg)
{
    uint8_t encoded_color = (uint8_t(bg) << 4) + uint8_t(fg);
    uint8_t seq[] = {0xc1, encoded_color};
    current_line().append(std::string(std::begin(seq), std::end(seq)));
}


void TextTerminal::set_attribute(uint8_t mask, uint8_t attr)
{
    // Overwrite the respective bits in m_attributes
    m_attributes = (m_attributes & ~mask) | (attr & mask);
    // Append control code
    uint8_t seq[] = {0xc0, m_attributes};
    current_line().append(std::string(std::begin(seq), std::end(seq)));
}


void TextTerminal::set_font_style(TextTerminal::FontStyle style)
{
    set_attribute(c_font_style_mask, static_cast<uint8_t>(style));
}


void TextTerminal::set_decoration(TextTerminal::Decoration decoration)
{
    set_attribute(c_decoration_mask, static_cast<uint8_t>(decoration) << c_decoration_shift);
}


void TextTerminal::set_mode(TextTerminal::Mode mode)
{
    set_attribute(c_mode_mask, static_cast<uint8_t>(mode) << c_mode_shift);
}


void TextTerminal::update(std::chrono::nanoseconds elapsed)
{
    if (m_bell_time > 0ns) {
        m_bell_time -= elapsed;
        // zero is special, make sure we don't end at that value
        if (m_bell_time == 0ns)
            m_bell_time = -1ns;
    }
}


void TextTerminal::resize(View& view)
{
    auto& font = theme().font();
    auto pxf = view.framebuffer_ratio();
    font.set_size(unsigned(m_font_size / pxf.y));
    m_cell_size = {font.max_advance() * pxf.x,
                   font.line_height() * pxf.y};
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
        int row = line_idx - m_buffer_offset;
        int column = 0;
        for (auto pos = line.cbegin(); pos != line.cend(); ) {
            if (*pos == '\xc0') {
                // decode attributes
                ++pos;
                auto style = static_cast<FontStyle>(*pos & c_font_style_mask);
                auto deco = static_cast<Decoration>((*pos & c_decoration_mask) >> c_decoration_shift);
                auto mode = static_cast<Mode>((*pos & c_mode_mask) >> c_mode_shift);
                font.set_style(style);
                (void) deco; // TODO
                (void) mode; // TODO
                ++pos;
                continue;
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

            // cursor = temporary reverse video
            // FIXME: draw cursor separately using "cursor shader"
            auto bg = boxes.fill_color();
            auto fg = sprites.color();
            if (row == m_cursor.y && column == m_cursor.x) {
                sprites.set_color(bg);
                boxes.set_fill_color(fg);
            }

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

            // revert colors after drawing cursor
            if (row == m_cursor.y && column == m_cursor.x) {
                sprites.set_color(fg);
                boxes.set_fill_color(bg);
            }

            pen.x += m_cell_size.x;
            ++column;
        }

        // draw the cursor separately in case it's out of line
        if (row == m_cursor.y && column <= m_cursor.x) {
            Rect_f rect { m_cell_size.x * m_cursor.x, pen.y, m_cell_size.x, m_cell_size.y };
            auto orig_color = boxes.fill_color();
            boxes.set_fill_color(c_colors_4bit[7]);
            boxes.add_rectangle(rect);
            boxes.set_fill_color(orig_color);
        }

        pen.x = 0;
        pen.y += m_cell_size.y;
    }

    boxes.draw(view, position());
    sprites.draw(view, position());

    if (m_bell_time > 0ns) {
        auto x = (float) duration_cast<milliseconds>(m_bell_time).count() / 500.f;
        graphics::Shape frame(Color::Transparent(), Color(1.f, 0.f, 0.f, x));
        frame.add_rectangle({{0, 0}, size()}, 0.01f);
        frame.draw(view, position());
        view.start_periodic_refresh();
    } else if (m_bell_time < 0ns) {
        m_bell_time = 0ns;
        view.stop_periodic_refresh();
    }
}


void TextTerminal::bell()
{
    m_bell_time = 500ms;
}


void TextTerminal::set_cursor_pos(util::Vec2i pos)
{
    // make sure new cursor position is not outside screen area
    m_cursor = {
        min(pos.x, m_cells.x),
        min(pos.y, m_cells.y),
    };
    // make sure there is a line in buffer at cursor position
    while (m_cursor.y >= int(m_buffer.size()) - m_buffer_offset) {
        m_buffer.add_line();
    }
}


}} // namespace xci::widgets
