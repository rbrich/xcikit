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


void terminal::Attributes::set_fg(terminal::Color8bit fg_color)
{
    set_bit(Fg);
    m_fg = ColorMode::Color8bit;
    m_fg_r = fg_color;
}


void terminal::Attributes::set_bg(terminal::Color8bit bg_color)
{
    set_bit(Bg);
    m_bg = ColorMode::Color8bit;
    m_bg_r = bg_color;
}


void terminal::Attributes::set_fg(Color24bit fg_color)
{
    set_bit(Fg);
    m_fg = ColorMode::Color24bit;
    m_fg_r = fg_color.r;
    m_fg_g = fg_color.g;
    m_fg_b = fg_color.b;
}



void terminal::Attributes::set_bg(Color24bit bg_color)
{
    set_bit(Bg);
    m_bg = ColorMode::Color24bit;
    m_bg_r = bg_color.r;
    m_bg_g = bg_color.g;
    m_bg_b = bg_color.b;
}


void terminal::Attributes::set_default_fg()
{
    set_bit(Fg);
    m_fg = ColorMode::ColorDefault;
}


void terminal::Attributes::set_default_bg()
{
    set_bit(Bg);
    m_bg = ColorMode::ColorDefault;
}



void terminal::Attributes::preceded_by(const terminal::Attributes& other)
{
    // If this and other set same attribute -> leave it out
    // If other sets some attribute, but this does not -> reset it
    if (m_set[Attr] && other.m_set[Attr] && m_attr == other.m_attr) {
        m_set[Attr] = false;
    } else if (!m_set[Attr] && other.m_set[Attr] && !other.m_attr.none()) {
        m_set[Attr] = true;
        m_attr.reset();
    }

    if (m_set[Fg] && other.m_set[Fg]
    && m_fg == other.m_fg
    && (m_fg == ColorMode::ColorDefault || m_fg_r == other.m_fg_r)
    && (m_fg != ColorMode::Color24bit || m_fg_g == other.m_fg_g)
    && (m_fg != ColorMode::Color24bit || m_fg_b == other.m_fg_b)) {
        m_set[Fg] = false;
    } else if (!m_set[Fg] && other.m_set[Fg] && other.m_fg != ColorMode::ColorDefault) {
        m_set[Fg] = true;
        m_fg = ColorMode::ColorDefault;
    }

    if (m_set[Bg] && other.m_set[Bg]
        && m_bg == other.m_bg
        && (m_bg == ColorMode::ColorDefault || m_bg_r == other.m_bg_r)
        && (m_bg != ColorMode::Color24bit || m_bg_g == other.m_bg_g)
        && (m_bg != ColorMode::Color24bit || m_bg_b == other.m_bg_b)) {
        m_set[Bg] = false;
    } else if (!m_set[Bg] && other.m_set[Bg] && other.m_bg != ColorMode::ColorDefault) {
        m_set[Bg] = true;
        m_bg = ColorMode::ColorDefault;
    }
}


std::string terminal::Attributes::encode() const
{
    std::string result;

    if (m_set[Attr]) {
        result.push_back(c_ctl_set_attrs);
        result.push_back(char(m_attr.to_ulong()));
    }

    if (m_set[Fg]) {
        switch (m_fg) {
            case ColorMode::ColorDefault:
                result.push_back(c_ctl_default_fg);
                break;
            case ColorMode::Color8bit:
                result.push_back(c_ctl_fg8bit);
                result.push_back(m_fg_r);
                break;
            case ColorMode::Color24bit:
                result.push_back(c_ctl_fg24bit);
                result.push_back(m_fg_r);
                result.push_back(m_fg_g);
                result.push_back(m_fg_b);
                break;
        }
    }

    if (m_set[Bg]) {
        switch (m_bg) {
            case ColorMode::ColorDefault:
                result.push_back(c_ctl_default_bg);
                break;
            case ColorMode::Color8bit:
                result.push_back(c_ctl_bg8bit);
                result.push_back(m_bg_r);
                break;
            case ColorMode::Color24bit:
                result.push_back(c_ctl_bg24bit);
                result.push_back(m_bg_r);
                result.push_back(m_bg_g);
                result.push_back(m_bg_b);
                break;
        }
    }

    return result;
}


size_t terminal::Attributes::decode(std::string_view sv)
{
    auto* it = sv.cbegin();
    while (it < sv.cend()) {
        if (*it <= c_ctl_first_introducer || *it > c_ctl_last_introducer)
            break;
        switch (*it) {
            case c_ctl_set_attrs:
                m_set[Attr] = true;
                m_attr = (*++it);
                break;
            case c_ctl_default_fg:
                set_default_fg();
                break;
            case c_ctl_default_bg:
                set_default_bg();
                break;
            case c_ctl_fg8bit:
                set_fg(Color8bit(*++it));
                break;
            case c_ctl_bg8bit:
                set_bg(Color8bit(*++it));
                break;
            case c_ctl_fg24bit:
                set_bit(Fg);
                m_fg = ColorMode::Color24bit;
                m_fg_r = uint8_t(*++it);
                m_fg_g = uint8_t(*++it);
                m_fg_b = uint8_t(*++it);
                break;
            case c_ctl_bg24bit:
                set_bit(Bg);
                m_bg = ColorMode::Color24bit;
                m_bg_r = uint8_t(*++it);
                m_bg_g = uint8_t(*++it);
                m_bg_b = uint8_t(*++it);
                break;
            default:
                log_error("terminal decode attributes: Encountered invalid code: {:02x}", int(*it));
                return it - sv.cbegin();
        }
        ++it;
    }
    return it - sv.cbegin();
}


Color terminal::Attributes::fg() const
{
    switch (m_fg) {
        case ColorMode::ColorDefault:   return Color(7);
        case ColorMode::Color8bit:      return Color(m_fg_r);
        case ColorMode::Color24bit:     return Color(m_fg_r, m_fg_g, m_fg_b);
    }
}


Color terminal::Attributes::bg() const
{
    switch (m_bg) {
        default:
        case ColorMode::ColorDefault:   return Color(0);
        case ColorMode::Color8bit:      return Color(m_bg_r);
        case ColorMode::Color24bit:     return Color(m_bg_r, m_bg_g, m_bg_b);
    }
}


// ------------------------------------------------------------------------


void terminal::Line::add_text(size_t pos, std::string_view sv, Attributes attr, bool insert)
{
    // Find `pos` in content (or end of content)
    auto* it = content_begin();
    auto skip = pos;
    Attributes attr_start;
    while (skip > 0 && it < content_end()) {
        if (Attributes::is_introducer(*it))
            it += attr_start.decode({it, size_t(content_end() - it)});
        it = utf8_next(it);
        --skip;
    }

    // Now we are at `pos` (or content end), but there might be some attribute
    auto* end = it;
    Attributes attr_end(attr_start);
    if (Attributes::is_introducer(*end))
        end += attr_end.decode({end, size_t(content_end() - end)});

    // Replace mode - find end of the place for new text (same length as `sv`)
    if (!insert) {
        auto len = utf8_length(sv);
        while (len > 0 && end < content_end()) {
            if (Attributes::is_introducer(*end))
                end += attr_end.decode({end, size_t(content_end() - end)});
            end = utf8_next(end);
            --len;
        }

        // Read also attributes after replace span
        // and unify them with attr_end
        if (Attributes::is_introducer(*end))
            end += attr_end.decode({end, size_t(content_end() - end)});
    }

    attr.preceded_by(attr_start);
    attr_end.preceded_by(attr);

    auto replace_start = it - content_begin() + skip;
    auto replace_length = end - it;
    m_content.append(skip, ' ');
    m_content.replace(replace_start, replace_length, sv.data(), sv.size());
    if (replace_start + sv.size() < m_content.size())
        m_content.insert(replace_start + sv.size(), attr_end.encode());
    m_content.insert(replace_start, attr.encode());
    //TRACE("content={}", escape(m_content));
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


// ------------------------------------------------------------------------


void terminal::Buffer::remove_lines(size_t start, size_t count)
{
    m_lines.erase(m_lines.begin() + start, m_lines.begin() + start + count);
}


// ------------------------------------------------------------------------


void TextTerminal::set_font_size(float size, bool scalable)
{
    m_font_size = size;
    m_font_scalable = scalable;
}


void TextTerminal::add_text(std::string_view text, bool insert)
{
    // Buffer for fragment of text without any attributes
    std::string buffer;
    size_t buffer_length = 0;
    auto flush_buffer = [this, &buffer, &buffer_length, insert]() {
        if (!buffer.empty()) {
            current_line().add_text(m_cursor.x, buffer, m_attrs, insert);
            m_cursor.x += buffer_length;
            buffer_length = 0;
            buffer.clear();
        }
    };
    // Add characters up to terminal width (columns)
    for (auto it = text.cbegin(); it != text.cend(); ) {
        // Special handling for newline character
        if (*it == '\n') {
            ++it;
            flush_buffer();
            break_line();
            new_line();
            continue;
        }

        // Check line length
        if (m_cursor.x + buffer_length >= m_cells.x - 1) {
            flush_buffer();
            new_line();
            continue;
        }

        // Add character to current line
        auto end_pos = utf8_next(it);
        buffer.append(std::string(text.substr(it - text.cbegin(), end_pos - it)));
        ++buffer_length;
        it = end_pos;
    }
    flush_buffer();
}


void TextTerminal::new_line()
{
    m_buffer->add_line();
    m_cursor.x = 0;
    if (m_cursor.y < m_cells.y - 1)
        ++m_cursor.y;
    else
        ++m_buffer_offset;
}


void TextTerminal::erase_page()
{
    m_buffer->remove_lines(size_t(m_buffer_offset), m_buffer->size() - m_buffer_offset);
    m_buffer->add_line();
    m_cursor = {0, 0};
}


void TextTerminal::erase_buffer()
{
    m_buffer->remove_lines(0, m_buffer->size());
    m_buffer->add_line();
    m_cursor = {0, 0};
}


std::unique_ptr<terminal::Buffer>
TextTerminal::set_buffer(std::unique_ptr<terminal::Buffer> new_buffer)
{
    assert(new_buffer);
    if (!new_buffer)
        return nullptr;
    std::swap(m_buffer, new_buffer);
    m_cursor = {0, 0};
    return new_buffer;
}


void TextTerminal::set_cursor_pos(util::Vec2u pos)
{
    // make sure new cursor position is not outside screen area
    m_cursor = {
        min(pos.x, m_cells.x),
        min(pos.y, m_cells.y),
    };
    // make sure there is a line in buffer at cursor position
    while (m_cursor.y >= int(m_buffer->size()) - m_buffer_offset) {
        m_buffer->add_line();
    }
    // scroll up if the cursor got out of page
    if (m_cursor.y >= m_cells.y) {
        m_buffer_offset += m_cursor.y - m_cells.y + 1;
        m_cursor.y = m_cells.y - 1;
    }
}


void TextTerminal::set_font_style(FontStyle style)
{
    m_attrs.set_italic(style == FontStyle::Italic || style == FontStyle::BoldItalic);
    m_attrs.set_bold(style == FontStyle::Bold || style == FontStyle::BoldItalic);
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


void TextTerminal::bell()
{
    m_bell_time = 500ms;
}


void TextTerminal::scrollback(double lines)
{
    if (m_scroll_offset == c_scroll_end) {
        m_scroll_offset = m_buffer_offset;
    }
    m_scroll_offset -= lines;
    if (m_scroll_offset > m_buffer_offset) {
        m_scroll_offset = m_buffer_offset;
    }
    if (m_scroll_offset < 0.0) {
        m_scroll_offset = 0.0;
    }
}


void TextTerminal::cancel_scrollback()
{
    m_scroll_offset = c_scroll_end;
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
    if (m_font_scalable)
        font.set_size(unsigned(m_font_size / pxf.y));
    else
        font.set_size(unsigned(m_font_size * view.framebuffer_to_screen_ratio().y));
    m_cell_size = {font.max_advance() * pxf.x,
                   font.line_height() * pxf.y};
    m_cells = {unsigned(size().x / m_cell_size.x),
               unsigned(size().y / m_cell_size.y)};
    log_debug("TextTerminal cells {}", m_cells);
}


void TextTerminal::draw(View& view, State state)
{
    auto& font = theme().font();
    auto pxf = view.framebuffer_ratio();
    if (m_font_scalable)
        font.set_size(unsigned(m_font_size / pxf.y));
    else
        font.set_size(unsigned(m_font_size * view.framebuffer_to_screen_ratio().y));
    graphics::ColoredSprites sprites(font.get_texture(), Color(7));
    graphics::Shape boxes(Color(0));

    Vec2f pen;
    size_t buffer_first, buffer_last;
    if (m_scroll_offset == c_scroll_end) {
        buffer_first = m_buffer_offset;
        buffer_last = std::min(m_buffer->size(), buffer_first + m_cells.y);
    } else {
        buffer_first = size_t(m_scroll_offset);
        buffer_last = std::min(m_buffer->size(), buffer_first + m_cells.y + 1);
        //pen.y -= (m_scroll_offset - buffer_first);
    }

    for (size_t line_idx = buffer_first; line_idx < buffer_last; line_idx++) {
        auto& line = (*m_buffer)[line_idx];
        size_t row = line_idx - m_buffer_offset;
        size_t column = 0;

        // Reset attributes
        font.set_style(text::FontStyle::Regular);
        sprites.set_color(Color(7));
        boxes.set_fill_color(Color(0));

        for (const char* it = line.content_begin(); it != line.content_end(); ) {
            if (*it == terminal::c_ctl_new_line) {
                ++it;
                continue;
            }
            if (terminal::Attributes::is_introducer(*it)) {
                terminal::Attributes attr;
                it += attr.decode({it, size_t(line.content_end() - it)});
                if (attr.has_attr()) {
                    font.set_style(attr.font_style());
                    // TODO:
                    //auto deco = static_cast<Decoration>((*it & c_decoration_mask) >> c_decoration_shift);
                    //auto mode = static_cast<Mode>((*it & c_mode_mask) >> c_mode_shift);
                    //(void) deco; // TODO
                    //(void) mode; // TODO
                }
                if (attr.has_fg())
                    sprites.set_color(attr.fg());
                if (attr.has_bg())
                    boxes.set_fill_color(attr.bg());
                continue;
            }

            // extract single UTF-8 character
            auto end_pos = utf8_next(it);
            auto ch = std::string_view(it, end_pos - it);
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


}} // namespace xci::widgets
