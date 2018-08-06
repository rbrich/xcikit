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
#include <xci/graphics/Sprites.h>
#include <xci/graphics/Shape.h>
#include <xci/util/string.h>
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


// Encoding of attributes byte
static constexpr uint8_t c_font_style_mask = 0b00000011;
static constexpr uint8_t c_decoration_mask = 0b00011100;
static constexpr uint8_t c_mode_mask       = 0b01100000;
static constexpr int c_decoration_shift = 2;
static constexpr int c_mode_shift = 5;


// Skip custom control seqs in UTF-8 string
const char* terminal::Attributes::skip(const char* introducer)
{
    auto* it = introducer;
    for (;;) {
        if (*it < 16 || *it > 31)  return it;
        else if (*it < 20)  it += 1;
        else if (*it < 24)  it += 2;
        else if (*it < 28)  it += 3;
        else if (*it < 32)  it += 4;
    }
}


const char* terminal::Attributes::rskip(const char* terminator)
{
    char size = *terminator;
    return terminator - size;
}


void terminal::Attributes::set_fg(terminal::Color8bit fg_color)
{
    set_bit(Fg);
    m_fg8bit = true;
    m_fg_r = fg_color;
}


void terminal::Attributes::set_bg(terminal::Color8bit bg_color)
{
    set_bit(Bg);
    m_bg8bit = true;
    m_bg_r = bg_color;
}


void terminal::Attributes::set_fg(Color24bit fg_color)
{
    set_bit(Fg);
    m_fg8bit = false;
    m_fg_r = fg_color.r;
    m_fg_g = fg_color.g;
    m_fg_b = fg_color.b;
}



void terminal::Attributes::set_bg(Color24bit bg_color)
{
    set_bit(Bg);
    m_bg8bit = false;
    m_bg_r = bg_color.r;
    m_bg_g = bg_color.g;
    m_bg_b = bg_color.b;
}


void terminal::Attributes::add_from(const terminal::Attributes& other)
{
    if (other.m_set[Fg]) {
        m_fg_r = other.m_fg_r;
        m_fg_g = other.m_fg_g;
        m_fg_b = other.m_fg_b;
        m_fg8bit = other.m_fg8bit;
    }
    if (other.m_set[Bg]) {
        m_bg_r = other.m_bg_r;
        m_bg_g = other.m_bg_g;
        m_bg_b = other.m_bg_b;
        m_bg8bit = other.m_bg8bit;
    }
    m_set = (m_set | other.m_set) & ~(other.m_reset);
    m_reset = (m_reset | other.m_reset) & ~(other.m_set);
}


std::string terminal::Attributes::encode() const
{
    assert((m_set & m_reset) == 0);

    std::string result;

    auto set_attrs = static_cast<uint8_t>(m_set.to_ulong() & c_attrs_mask);
    if (set_attrs) {
        result.push_back(c_ctl_set_attrs);
        result.push_back(set_attrs);
    }
    auto reset_attrs = static_cast<uint8_t>(m_reset.to_ulong() & c_attrs_mask);
    if (reset_attrs) {
        result.push_back(c_ctl_reset_attrs);
        result.push_back(reset_attrs);
    }

    if (m_reset[Fg]) {
        result.push_back(c_ctl_default_fg);
    }
    if (m_reset[Bg]) {
        result.push_back(c_ctl_default_bg);
    }
    if (m_fg8bit && m_set[Fg]) {
        result.push_back(c_ctl_fg8bit);
        result.push_back(m_fg_r);
    }
    if (m_bg8bit && m_set[Bg]) {
        result.push_back(c_ctl_bg8bit);
        result.push_back(m_bg_r);
    }
    if (!m_fg8bit && m_set[Fg]) {
        result.push_back(c_ctl_fg24bit);
        result.push_back(m_fg_r);
        result.push_back(m_fg_g);
        result.push_back(m_fg_b);
    }
    if (!m_bg8bit && m_set[Bg]) {
        result.push_back(c_ctl_bg24bit);
        result.push_back(m_bg_r);
        result.push_back(m_bg_g);
        result.push_back(m_bg_b);
    }

    if (result.empty())
        return result;

    assert(result.size() <= c_ctl_max_terminator);
    result.push_back(uint8_t(result.size()));
    return result;
}


size_t terminal::Attributes::decode(std::string_view sv)
{
    m_set.reset();
    m_reset.reset();
    auto* it = sv.cbegin();
    while (it < sv.cend()) {
        if (*it <= c_ctl_max_terminator || *it > c_ctl_max_introducer)
            break;
        switch (*it) {
            case c_ctl_set_attrs: {
                auto a = (*++it) & c_attrs_mask;
                m_set |= a;
                m_reset &= ~a;
                break;
            }
            case c_ctl_reset_attrs: {
                auto a = (*++it) & c_attrs_mask;
                m_reset |= a;
                m_set &= ~a;
                break;
            }
            case c_ctl_default_fg:
                reset_fg();
                ++it;
                break;
            case c_ctl_default_bg:
                reset_bg();
                ++it;
                break;
            case c_ctl_fg8bit:
                set_fg(Color8bit(*++it));
                ++it;
                break;
            case c_ctl_bg8bit:
                set_bg(Color8bit(*++it));
                ++it;
                break;
            case c_ctl_fg24bit:
                set_bit(Fg);
                m_fg8bit = false;
                m_fg_r = uint8_t(*++it);
                m_fg_g = uint8_t(*++it);
                m_fg_b = uint8_t(*++it);
                ++it;
                break;
            case c_ctl_bg24bit:
                set_bit(Bg);
                m_bg8bit = false;
                m_bg_r = uint8_t(*++it);
                m_bg_g = uint8_t(*++it);
                m_bg_b = uint8_t(*++it);
                ++it;
                break;
            default:
                log_error("terminal decode attributes: Encountered invalid code: {:02x}", int(*it));
                return it - sv.cbegin();
        }
    }
    if (*it > 0 && *it <= c_ctl_max_terminator)
        ++it;
    return it - sv.cbegin();
}


// ------------------------------------------------------------------------


void terminal::Line::set_attr(int pos, const terminal::Attributes& new_attr)
{
    const char* it = content_begin();
    while (pos > 0 && it < content_end()) {
        it = Attributes::skip(it);
        it = utf8_next(it);
    }
    Attributes attr;
    size_t old_length = attr.decode({it, size_t(content_end() - it)});
    attr.add_from(new_attr);
    m_content.replace(it - content_begin(), old_length, new_attr.encode());
}


void terminal::Line::insert_text(int pos, std::string_view sv)
{
    int current = 0;
    for (const char* it = m_content.data();
            it != m_content.data() + m_content.size();
            it = utf8_next(it))
    {
        it = Attributes::skip(it);
        if (pos == current) {
            m_content.insert(it - m_content.data(), sv.data(), sv.size());
            return;
        }
        ++current;
    }
    assert(pos >= current);
    m_content.append(sv.data(), sv.size());
}


void terminal::Line::replace_text(int pos, std::string_view sv)
{
    int current = 0;
    for (const char* it = Attributes::skip(m_content.data());
            it != m_content.data() + m_content.size();
            it = Attributes::skip(utf8_next(it)))
    {
        if (pos == current) {
            auto end = it;
            for (size_t i = 0; i < utf8_length(sv); i++) {
                end = utf8_next(end);
            }
            m_content.replace(it - m_content.data(), end - it, sv.data(), sv.size());
            return;
        }
        ++current;
    }
    assert(pos >= current);
    m_content.append(sv.data(), sv.size());
}


void terminal::Line::erase_text(int first, int num)
{
    TRACE("first={}, num={}, line size={}", first, num, m_content.size());
    int current = 0;
    const char* erase_start = m_content.data() + m_content.size();
    const char* erase_end = erase_start;
    for (const char* it = m_content.data();
            it != m_content.data() + m_content.size();
            it = utf8_next(it))
    {
        it = Attributes::skip(it);
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
    if (erase_end != m_content.data() + m_content.size())
        m_content.erase(erase_start - m_content.data(), erase_end - erase_start);
    else
        m_content.erase(erase_start - m_content.data());
}


void terminal::Line::set_line_break()
{
    m_content.push_back(c_ctl_new_line);
}


int terminal::Line::length() const
{
    int length = 0;
    for (const char* it = m_content.data(); it != m_content.data() + m_content.size(); it = utf8_next(it)) {
        it = Attributes::skip(it);
        if (*it != '\n')
            ++length;
    }
    return length;
}


void terminal::Buffer::remove_lines(size_t start, size_t count)
{
    m_lines.erase(m_lines.begin() + start, m_lines.begin() + start + count);
}


void TextTerminal::add_text(std::string_view text)
{
    // Add characters up to terminal width (columns)
    for (auto it = text.cbegin(); it != text.cend(); ) {
        // Special handling for newline character
        if (*it == '\n') {
            ++it;
            break_line();
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
        current_line().replace_text(m_cursor.x, text.substr(it - text.cbegin(), end_pos - it));
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
    terminal::Attributes attr;
    attr.set_fg(fg_color);
    current_line().set_attr(m_cursor.x, attr);
}


void TextTerminal::set_bg(Color8bit bg_color)
{
    terminal::Attributes attr;
    attr.set_bg(bg_color);
    current_line().set_attr(m_cursor.x, attr);
}


void TextTerminal::set_fg(Color24bit fg_color)
{
    terminal::Attributes attr;
    attr.set_fg(fg_color);
    current_line().set_attr(m_cursor.x, attr);
}


void TextTerminal::set_bg(Color24bit bg_color)
{
    terminal::Attributes attr;
    attr.set_bg(bg_color);
    current_line().set_attr(m_cursor.x, attr);
}


void TextTerminal::set_font_style(FontStyle style)
{
    terminal::Attributes attr;
    attr.set_italic(style == FontStyle::Italic || style == FontStyle::BoldItalic);
    attr.set_bold(style == FontStyle::Bold || style == FontStyle::BoldItalic);
    current_line().set_attr(m_cursor.x, attr);
}


void TextTerminal::set_decoration(TextTerminal::Decoration decoration)
{
    // TODO
    //set_attribute(c_decoration_mask, static_cast<uint8_t>(decoration) << c_decoration_shift);
}


void TextTerminal::set_mode(TextTerminal::Mode mode)
{
    // TODO
    //set_attribute(c_mode_mask, static_cast<uint8_t>(mode) << c_mode_shift);
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
        for (const char* it = line.data(); it != line.data() + line.size(); ) {
            if (uint8_t(*it) == terminal::c_ctl_set_attrs) {
                // decode attributes
                ++it;
                auto style = static_cast<FontStyle>(*it & c_font_style_mask);
                auto deco = static_cast<Decoration>((*it & c_decoration_mask) >> c_decoration_shift);
                auto mode = static_cast<Mode>((*it & c_mode_mask) >> c_mode_shift);
                font.set_style(style);
                (void) deco; // TODO
                (void) mode; // TODO
                ++it;
                continue;
            }

            if (uint8_t(*it) == terminal::c_ctl_fg8bit){
                // decode 8-bit fg color
                ++it;
                sprites.set_color(Color(uint8_t(*it)));
                ++it;
                continue;
            }

            if (uint8_t(*it) == terminal::c_ctl_bg8bit){
                // decode 8-bit bg color
                ++it;
                boxes.set_fill_color(Color(uint8_t(*it)));
                ++it;
                continue;
            }

            if (uint8_t(*it) == terminal::c_ctl_fg24bit){
                // decode 24-bit fg color
                auto r = *++it;
                auto g = *++it;
                auto b = *++it;
                Color fg(r, g, b);
                sprites.set_color(fg);
                ++it;
                continue;
            }

            if (uint8_t(*it) == terminal::c_ctl_bg24bit){
                // decode 24-bit bg color
                auto r = *++it;
                auto g = *++it;
                auto b = *++it;
                Color bg(r, g, b);
                boxes.set_fill_color(bg);
                ++it;
                continue;
            }

            // extract single UTF-8 character
            auto end_pos = utf8_next(it);
            auto ch = line.substr(it - line.data(), end_pos - it);
            it = end_pos;

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


void TextTerminal::erase_page()
{
    m_buffer.remove_lines(size_t(m_buffer_offset), m_buffer.size() - m_buffer_offset);
    m_buffer.add_line();
    m_cursor.y = 0;
}


void TextTerminal::erase_buffer()
{
    m_buffer.remove_lines(0, m_buffer.size());
    m_buffer.add_line();
    m_cursor.y = 0;
}


}} // namespace xci::widgets
