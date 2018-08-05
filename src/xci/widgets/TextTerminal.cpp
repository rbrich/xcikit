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


// Custom control sequence introducers
// (These are added to text internally)
static constexpr uint8_t c_ctl_attrs = 0xc0;
static constexpr uint8_t c_ctl_fg8bit = 0xf5;
static constexpr uint8_t c_ctl_bg8bit = 0xf6;
static constexpr uint8_t c_ctl_fg24bit = 0xf7;
static constexpr uint8_t c_ctl_bg24bit = 0xf8;

// Length of custom control sequences
static constexpr size_t c_len_attrs = 2;
static constexpr size_t c_len_fg8bit = 2;
static constexpr size_t c_len_bg8bit = 2;
static constexpr size_t c_len_fg24bit = 4;
static constexpr size_t c_len_bg24bit = 4;

// Encoding of attributes byte
static constexpr uint8_t c_font_style_mask = 0b00000011;
static constexpr uint8_t c_decoration_mask = 0b00011100;
static constexpr uint8_t c_mode_mask       = 0b01100000;
static constexpr int c_decoration_shift = 2;
static constexpr int c_mode_shift = 5;


// Skip custom control seqs in UTF-8 string
static std::string::const_iterator
ctl_skip(std::string::const_iterator pos)
{
    for (;;) {
        switch (static_cast<uint8_t>(*pos)) {
            case c_ctl_attrs: pos += c_len_attrs; break;
            case c_ctl_fg8bit: pos += c_len_fg8bit; break;
            case c_ctl_bg8bit: pos += c_len_bg8bit; break;
            case c_ctl_fg24bit: pos += c_len_fg24bit; break;
            case c_ctl_bg24bit: pos += c_len_bg24bit; break;
            default:
                return pos;
        }
    }
}


void terminal::Line::insert(int pos, std::string_view string)
{
    int current = 0;
    for (auto it = m_content.cbegin(); it != m_content.cend(); it = utf8_next(it)) {
        it = ctl_skip(it);
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
        it = ctl_skip(it);
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
    TRACE("first={}, num={}, line size={}", first, num, m_content.size());
    int current = 0;
    auto erase_start = m_content.cend();
    auto erase_end = m_content.cend();
    for (auto it = m_content.cbegin(); it < m_content.cend(); it = utf8_next(it)) {
        it = ctl_skip(it);
        if (current == first) {
            erase_start = it;
        }
        if (current > first) {
            if (num > 0)
                --num;
            else {
                erase_end = it;
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
    for (auto it = m_content.cbegin(); it != m_content.cend(); it = utf8_next(it)) {
        it = ctl_skip(it);
        if (*it != '\n')
            ++length;
    }
    return length;
}


void TextTerminal::add_text(const std::string& text)
{
    // Add characters up to terminal width (columns)
    for (auto it = text.cbegin(); it != text.cend(); ) {
        // Special handling for newline character
        if (*it == '\n') {
            ++it;
            new_line();
            continue;
        }

        // Check line length
        if (m_cursor.x >= m_cells.x) {
            new_line();
            continue;
        }

        // Add character to current line
        auto end_pos = utf8_next(it);
        current_line().replace(m_cursor.x, text.substr(it - text.cbegin(), end_pos - it));
        it = end_pos;
        ++m_cursor.x;
    }
}


void TextTerminal::new_line()
{
    m_buffer.add_line();
    ++m_cursor.y;
    m_cursor.x = 0;
}


void TextTerminal::set_fg(Color8bit fg_color)
{
    uint8_t seq[] = {c_ctl_fg8bit, fg_color};
    current_line().append(std::string_view((char*)seq, sizeof(seq)));
}


void TextTerminal::set_bg(Color8bit bg_color)
{
    uint8_t seq[] = {c_ctl_bg8bit, bg_color};
    current_line().append(std::string_view((char*)seq, sizeof(seq)));
}


void TextTerminal::set_fg(Color24bit fg_color)
{
    uint8_t seq[] = {c_ctl_fg24bit, fg_color.r, fg_color.g, fg_color.b};
    current_line().append(std::string_view((char*)seq, sizeof(seq)));
}


void TextTerminal::set_bg(Color24bit bg_color)
{
    uint8_t seq[] = {c_ctl_bg24bit, bg_color.r, bg_color.g, bg_color.b};
    current_line().append(std::string_view((char*)seq, sizeof(seq)));
}


void TextTerminal::set_attribute(uint8_t mask, uint8_t attr)
{
    // Overwrite the respective bits in m_attributes
    m_attributes = (m_attributes & ~mask) | (attr & mask);
    // Append control code
    uint8_t seq[] = {c_ctl_attrs, m_attributes};
    current_line().append(std::string_view((char*)seq, sizeof(seq)));
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

    graphics::ColoredSprites sprites(font.get_texture(), Color(7));
    graphics::Shape boxes(Color(0));

    Vec2f pen;
    auto buffer_last = std::min(int(m_buffer.size()), m_buffer_offset + m_cells.y);
    for (int line_idx = m_buffer_offset; line_idx < buffer_last; line_idx++) {
        auto& line = m_buffer[line_idx].content();
        int row = line_idx - m_buffer_offset;
        int column = 0;
        for (auto pos = line.cbegin(); pos != line.cend(); ) {
            if (uint8_t(*pos) == c_ctl_attrs) {
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

            if (uint8_t(*pos) == c_ctl_fg8bit){
                // decode 8-bit fg color
                ++pos;
                sprites.set_color(Color(uint8_t(*pos)));
                ++pos;
                continue;
            }

            if (uint8_t(*pos) == c_ctl_bg8bit){
                // decode 8-bit bg color
                ++pos;
                boxes.set_fill_color(Color(uint8_t(*pos)));
                ++pos;
                continue;
            }

            if (uint8_t(*pos) == c_ctl_fg24bit){
                // decode 24-bit fg color
                Color fg(*++pos, *++pos, *++pos);
                sprites.set_color(fg);
                ++pos;
                continue;
            }

            if (uint8_t(*pos) == c_ctl_bg24bit){
                // decode 24-bit bg color
                Color bg(*++pos, *++pos, *++pos);
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
            boxes.set_fill_color(Color(7));
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
