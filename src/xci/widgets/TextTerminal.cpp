// TextTerminal.cpp created on 2018-07-19 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "TextTerminal.h"
#include <xci/core/string.h>
#include <xci/core/log.h>

namespace xci::widgets {

using namespace xci::graphics;
using namespace xci::graphics::unit_literals;
using namespace xci::core;
using xci::text::CodePoint;

using std::string_view;
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
    set_bit(FlagFg);
    m_fg = ColorMode::Color8bit;
    m_fg_r = fg_color;
}


void terminal::Attributes::set_bg(terminal::Color8bit bg_color)
{
    set_bit(FlagBg);
    m_bg = ColorMode::Color8bit;
    m_bg_r = bg_color;
}


void terminal::Attributes::set_fg(Color24bit fg_color)
{
    set_bit(FlagFg);
    m_fg = ColorMode::Color24bit;
    m_fg_r = fg_color.r;
    m_fg_g = fg_color.g;
    m_fg_b = fg_color.b;
}



void terminal::Attributes::set_bg(Color24bit bg_color)
{
    set_bit(FlagBg);
    m_bg = ColorMode::Color24bit;
    m_bg_r = bg_color.r;
    m_bg_g = bg_color.g;
    m_bg_b = bg_color.b;
}


void terminal::Attributes::set_default_fg()
{
    set_bit(FlagFg);
    m_fg = ColorMode::ColorDefault;
}


void terminal::Attributes::set_default_bg()
{
    set_bit(FlagBg);
    m_bg = ColorMode::ColorDefault;
}


void terminal::Attributes::set_font_style(text::FontStyle style)
{
    set_bit(FlagFontStyle);
    m_font_style = style;
}


void terminal::Attributes::set_mode(terminal::Mode mode)
{
    set_bit(FlagMode);
    m_mode = mode;
}


void terminal::Attributes::set_decoration(terminal::Decoration decoration)
{
    set_bit(FlagDecoration);
    m_decoration = decoration;
}


void terminal::Attributes::preceded_by(const terminal::Attributes& other)
{
    // If this and other set the same attribute -> leave it out
    // If other sets some attribute, but this does not -> reset it
    // Otherwise, both this and other set the attribute -> set
    if (m_set[FlagFontStyle] && other.m_set[FlagFontStyle] && m_font_style == other.m_font_style) {
        m_set[FlagFontStyle] = false;
    } else if (!m_set[FlagFontStyle] && other.m_set[FlagFontStyle] && other.m_font_style != text::FontStyle::Regular) {
        set_font_style(text::FontStyle::Regular);
    }

    if (m_set[FlagDecoration] && other.m_set[FlagDecoration] && m_decoration == other.m_decoration) {
        m_set[FlagDecoration] = false;
    } else if (!m_set[FlagDecoration] && other.m_set[FlagDecoration] && other.m_decoration != Decoration::None) {
        set_decoration(Decoration::None);
    }

    if (m_set[FlagMode] && other.m_set[FlagMode] && m_mode == other.m_mode) {
        m_set[FlagMode] = false;
    } else if (!m_set[FlagMode] && other.m_set[FlagMode] && other.m_mode != Mode::Normal) {
        set_mode(Mode::Normal);
    }

    if (m_set[FlagFg] && other.m_set[FlagFg]
    && m_fg == other.m_fg
    && (m_fg == ColorMode::ColorDefault || m_fg_r == other.m_fg_r)
    && (m_fg != ColorMode::Color24bit || m_fg_g == other.m_fg_g)
    && (m_fg != ColorMode::Color24bit || m_fg_b == other.m_fg_b)) {
        m_set[FlagFg] = false;  // no change
    } else if (!m_set[FlagFg] && other.m_set[FlagFg] && other.m_fg != ColorMode::ColorDefault) {
        set_default_fg();
    }

    if (m_set[FlagBg] && other.m_set[FlagBg]
    && m_bg == other.m_bg
    && (m_bg == ColorMode::ColorDefault || m_bg_r == other.m_bg_r)
    && (m_bg != ColorMode::Color24bit || m_bg_g == other.m_bg_g)
    && (m_bg != ColorMode::Color24bit || m_bg_b == other.m_bg_b)) {
        m_set[FlagBg] = false;  // no change
    } else if (!m_set[FlagBg] && other.m_set[FlagBg] && other.m_bg != ColorMode::ColorDefault) {
        set_default_bg();
    }
}


std::string terminal::Attributes::encode() const
{
    std::string result;

    if (m_set[FlagFontStyle]) {
        result.push_back(ctl::font_style);
        result.push_back(char(m_font_style));
    }

    if (m_set[FlagDecoration]) {
        result.push_back(ctl::decoration);
        result.push_back(char(m_decoration));
    }

    if (m_set[FlagMode]) {
        result.push_back(ctl::mode);
        result.push_back(char(m_mode));
    }

    if (m_set[FlagFg]) {
        switch (m_fg) {
            case ColorMode::ColorDefault:
                result.push_back(ctl::default_fg);
                break;
            case ColorMode::Color8bit:
                result.push_back(ctl::fg8bit);
                result.push_back(m_fg_r);
                break;
            case ColorMode::Color24bit:
                result.push_back(ctl::fg24bit);
                result.push_back(m_fg_r);
                result.push_back(m_fg_g);
                result.push_back(m_fg_b);
                break;
        }
    }

    if (m_set[FlagBg]) {
        switch (m_bg) {
            case ColorMode::ColorDefault:
                result.push_back(ctl::default_bg);
                break;
            case ColorMode::Color8bit:
                result.push_back(ctl::bg8bit);
                result.push_back(m_bg_r);
                break;
            case ColorMode::Color24bit:
                result.push_back(ctl::bg24bit);
                result.push_back(m_bg_r);
                result.push_back(m_bg_g);
                result.push_back(m_bg_b);
                break;
        }
    }

    return result;
}


size_t terminal::Attributes::decode(string_view sv)
{
    auto it = sv.cbegin();
    while (it < sv.cend()) {
        if (*it < ctl::first_introducer || *it > ctl::last_introducer)
            break;
        switch (*it) {
            case ctl::font_style:
                m_set[FlagFontStyle] = true;
                m_font_style = text::FontStyle(*++it);
                break;
            case ctl::decoration:
                m_set[FlagDecoration] = true;
                m_decoration = Decoration(*++it);
                break;
            case ctl::mode:
                m_set[FlagMode] = true;
                m_mode = Mode(*++it);
                break;
            case ctl::default_fg:
                set_default_fg();
                break;
            case ctl::default_bg:
                set_default_bg();
                break;
            case ctl::fg8bit:
                set_fg(Color8bit(*++it));
                break;
            case ctl::bg8bit:
                set_bg(Color8bit(*++it));
                break;
            case ctl::fg24bit:
                set_bit(FlagFg);
                m_fg = ColorMode::Color24bit;
                m_fg_r = uint8_t(*++it);
                m_fg_g = uint8_t(*++it);
                m_fg_b = uint8_t(*++it);
                break;
            case ctl::bg24bit:
                set_bit(FlagBg);
                m_bg = ColorMode::Color24bit;
                m_bg_r = uint8_t(*++it);
                m_bg_g = uint8_t(*++it);
                m_bg_b = uint8_t(*++it);
                break;
            default:
                log::error("terminal decode attributes: Encountered invalid code: {:02x}", int(*it));
                return it - sv.cbegin();
        }
        ++it;
    }
    return it - sv.cbegin();
}


Color terminal::Attributes::fg() const
{
    if (!has_fg())
        return Color(7);
    switch (m_fg) {
        default:
        case ColorMode::ColorDefault:   return Color(7);
        case ColorMode::Color8bit:      return Color(m_fg_r);
        case ColorMode::Color24bit:     return Color(m_fg_r, m_fg_g, m_fg_b);
    }
}


Color terminal::Attributes::bg() const
{
    if (!has_bg())
        return Color(0);
    switch (m_bg) {
        default:
        case ColorMode::ColorDefault:   return Color(0);
        case ColorMode::Color8bit:      return Color(m_bg_r);
        case ColorMode::Color24bit:     return Color(m_bg_r, m_bg_g, m_bg_b);
    }
}


void terminal::Attributes::render(terminal::Renderer& renderer)
{
    if (has_font_style())
        renderer.set_font_style(m_font_style);
    if (has_decoration())
        renderer.set_decoration(m_decoration);
    if (has_mode())
        renderer.set_mode(m_mode);
    if (has_fg()) {
        switch (m_fg) {
            default:
            case ColorMode::ColorDefault:
                renderer.set_default_fg_color();
                break;
            case ColorMode::Color8bit:
                renderer.set_fg_color(m_fg_r);
                break;
            case ColorMode::Color24bit:
                renderer.set_fg_color(Color(m_fg_r, m_fg_g, m_fg_b));
                break;
        }
    }
    if (has_bg()) {
        switch (m_bg) {
            default:
            case ColorMode::ColorDefault:
                renderer.set_default_bg_color();
                break;
            case ColorMode::Color8bit:
                renderer.set_bg_color(m_bg_r);
                break;
            case ColorMode::Color24bit:
                renderer.set_bg_color(Color(m_bg_r, m_bg_g, m_bg_b));
                break;
        }
    }
}


// ------------------------------------------------------------------------


void terminal::Line::clear(const terminal::Attributes& attr)
{
    m_content.clear();
    m_content.append(attr.encode());
    m_flags[BlankLine] = true;
}


size_t terminal::Line::content_skip(size_t skip, size_t start, Attributes& attr)
{
    auto pos = start;
    while (skip > 0 && pos < m_content.size()) {
        if (Attributes::is_introducer(m_content[pos])) {
            pos += attr.decode(std::string_view{m_content}.substr(pos));
            continue;
        }
        if (m_content[pos] == ctl::blanks) {
            ++pos;
            auto num_blanks = (size_t) m_content[pos];
            if (skip >= num_blanks) {
                skip -= num_blanks;
                ++pos;
                continue;
            } else { // skip < num
                // Split blanks into two groups
                // Write back blanks before pos
                m_content[pos] = uint8_t(skip);
                ++pos;
                // Write rest of blanks after pos
                num_blanks -= skip;
                skip = 0;
                uint8_t blank_rest[2] = {ctl::blanks, uint8_t(num_blanks)};
                m_content.insert(pos, (char*)blank_rest, sizeof(blank_rest));
                break;
            }
        }
        pos = utf8_next(m_content.cbegin() + pos) - m_content.cbegin();
        --skip;
    }
    if (skip > 0) {
        uint8_t blank_skip[2] = {ctl::blanks, uint8_t(skip)};
        m_content.insert(pos, (char*)blank_skip, sizeof(blank_skip));
        return pos + sizeof(blank_skip);
    } else {
        return pos;
    }
}


void terminal::Line::add_text(size_t pos, string_view sv, Attributes attr, bool insert)
{
    // Find `pos` in content
    Attributes attr_start;
    auto start = content_skip(pos, 0, attr_start);

    // Now we are at `pos` (or content end), but there might be some attribute
    Attributes attr_end(attr_start);
    auto end = start;
    if (end < m_content.size() && Attributes::is_introducer(m_content[end]))
        end += attr_end.decode(std::string_view{m_content}.substr(end));

    // Replace mode - find end of the place for new text (same length as `sv`)
    if (!insert) {
        auto len = utf8_length(sv);
        end = content_skip(len, end, attr_end);

        // Read also attributes after replace span
        // and unify them with attr_end
        if (end < m_content.size() && Attributes::is_introducer(m_content[end]))
            end += attr_end.decode(std::string_view{m_content}.substr(end));
    }

    attr.preceded_by(attr_start);
    attr_end.preceded_by(attr);

    auto replace_length = end - start;

    // Write attributes
    auto repl = attr.encode();

    // Write text
    repl.append(sv.data(), sv.size());

    // Write original attributes
    repl.append(attr_end.encode());

    // Replace original text
    m_content.replace(start, replace_length, repl);

    //TRACE("content={}", escape(m_content));
}


void terminal::Line::delete_text(size_t first, size_t num)
{
    TRACE("first={}, num={}, line size={}", first, num, m_content.size());
    if (!num)
        return;

    // Find `first` in content
    Attributes attr_start;
    auto start = content_skip(first, 0, attr_start);

    // Find `first` + `num` in content
    Attributes attr_end(attr_start);
    auto end = content_skip(num, start, attr_end);

    // Read also attributes after delete span
    // and unify them with attr_end
    if (end < m_content.size() && Attributes::is_introducer(m_content[end]))
        end += attr_end.decode(std::string_view{m_content}.substr(end));

    attr_end.preceded_by(attr_start);
    m_content.erase(start, end - start);
    m_content.insert(start, attr_end.encode());
}


void terminal::Line::erase_text(size_t first, size_t num, Attributes attr)
{
    // Find `first` in content
    Attributes attr_start;
    auto start = content_skip(first, 0, attr_start);

    // Find `first` + `num` in content
    Attributes attr_end(attr_start);
    auto end = content_skip(num, start, attr_end);

    // Read also attributes after delete span
    // and unify them with attr_end
    if (end < m_content.size() && Attributes::is_introducer(m_content[end]))
        end += attr_end.decode(std::string_view{m_content}.substr(end));

    attr.preceded_by(attr_start);
    attr_end.preceded_by(attr);

    // Write attributes
    std::string repl = attr.encode();

    if (num != 0) {
        // Write blanks
        while (num > 0) {
            auto num_blanks = std::min(num, size_t(UINT8_MAX));
            repl.push_back(ctl::blanks);
            repl.push_back(uint8_t(num_blanks));
            num -= num_blanks;
        }
        // Write back original attributes
        repl += attr_end.encode();
    } else {
        // Write special control for blanked rest of line
        m_flags[BlankLine] = true;
        end = m_content.size();
    }

    // Replace text with blanks
    m_content.replace(start, end - start, repl);
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


void terminal::Line::render(Renderer& renderer)
{
    for (auto it = m_content.cbegin(); it != m_content.cend(); ) {
        if (*it == terminal::ctl::blanks) {
            auto num = uint8_t(*++it);
            renderer.draw_blanks(num);
            ++it;
            continue;
        }
        if (terminal::Attributes::is_introducer(*it)) {
            terminal::Attributes attr;
            it += attr.decode({&*it, size_t(m_content.cend() - it)});
            attr.render(renderer);
            continue;
        }

        // extract single UTF-8 character
        renderer.draw_char(utf8_codepoint(&*it));
        it = utf8_next(it);
    }
}


// ------------------------------------------------------------------------


void terminal::Buffer::add_line()
{
    m_lines.emplace_back();
}


void terminal::Buffer::remove_lines(size_t start, size_t count)
{
    m_lines.erase(m_lines.begin() + start, m_lines.begin() + start + count);
}


// ------------------------------------------------------------------------


void terminal::Caret::update(View& view, const ViewportRect& rect)
{
    view.finish_draw();

    auto x1 = rect.x;
    auto y1 = rect.y;
    auto x2 = rect.x + rect.w;
    auto y2 = rect.y + rect.h;
    auto outline_thickness = view.size_to_viewport(1.0_sc);
    float tx = 2.0f * outline_thickness.value / rect.w.value;
    float ty = 2.0f * outline_thickness.value / rect.h.value;
    float ix = 1.0f + tx / (1.0f - tx);
    float iy = 1.0f + ty / (1.0f - ty);

    m_quad.clear();
    m_quad.begin_primitive();
    m_quad.add_vertex({x1, y1}, -ix, -iy);
    m_quad.add_vertex({x1, y2}, -ix, +iy);
    m_quad.add_vertex({x2, y2}, +ix, +iy);
    m_quad.add_vertex({x2, y1}, +ix, -iy);
    m_quad.end_primitive();

    // pure white
    m_quad.add_uniform(1, {0.7, 0.7, 0.7}, {0.7, 0.7, 0.7});
    // green
    //m_quad.add_uniform(1, {0.0, 0.7, 0.3}, Color::Green());
    // yellow
    //m_quad.add_uniform(1, {0.7, 0.7, 0.3}, Color::Yellow());

    m_quad.set_shader(m_shader);
    m_quad.set_blend(BlendFunc::InverseVideo);
    m_quad.update();
}


void terminal::Caret::draw(View& view, const ViewportCoords& pos)
{
    m_quad.draw(view, pos);
}


// ------------------------------------------------------------------------


TextTerminal::TextTerminal(Theme& theme)
        : Widget(theme),
          m_sprites(theme.renderer(), theme.font().texture(), Color(7)),
          m_boxes(theme.renderer(), Color(0)),
          m_caret(theme.renderer()),
          m_frame(theme.renderer(), Color::Transparent(), Color::Transparent())
{
    set_focusable(true);
}


void TextTerminal::set_font_size(ViewportUnits size)
{
    m_font_size_requested = size;
    m_font_size = 0;
}


void TextTerminal::add_text(string_view text, bool insert, bool wrap)
{
    // Buffer for fragment of text without any attributes
    std::string buffer;
    size_t buffer_length = 0;
    auto flush_buffer = [this, &buffer, &buffer_length, insert]() {
        if (!buffer.empty()) {
            current_line().add_text(m_cursor.x, buffer, m_attrs, insert);
            m_cursor.x += (uint32_t) buffer_length;
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
        if (m_cursor.x + buffer_length >= m_cells.x) {
            flush_buffer();
            if (wrap)
                new_line();
            else
                m_cursor.x = m_cells.x - 1;
            continue;
        }

        // Add character to current line
        auto end_pos = utf8_next(it);
        std::string ch_str(text.substr(it - text.cbegin(), end_pos - it));

        if (!wrap && buffer.empty() && m_cursor.x == m_cells.x - 1) {
            // nowrap mode - keep overwriting last cell
            current_line().add_text(m_cursor.x, ch_str, m_attrs, /*insert=*/false);
        } else {
            buffer.append(ch_str);
            ++buffer_length;
        }

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


void TextTerminal::erase_in_line(size_t first, size_t num)
{
    if (num > size_in_cells().x - first)
        num = 0;
    current_line().erase_text(first, num, m_attrs);
}


void TextTerminal::erase_to_end_of_page()
{
    size_t line_from = m_buffer_offset + m_cursor.y + 1;
    if (line_from < m_buffer->size())
        m_buffer->remove_lines(line_from, m_buffer->size() - line_from);
    current_line().erase_text(cursor_pos().x, 0, m_attrs);
    current_line().set_blank_page();
}


void TextTerminal::erase_to_cursor()
{
    for (size_t line = m_buffer_offset; line < m_buffer_offset + m_cursor.y; line++) {
        (*m_buffer)[line].clear(m_attrs);
    }
    current_line().erase_text(0, cursor_pos().x + 1, m_attrs);
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
    m_buffer_offset = 0;
}


void TextTerminal::erase_scrollback()
{
    if (m_buffer_offset > 0) {
        m_buffer->remove_lines(0, m_buffer_offset);
        m_buffer_offset = 0;
    }
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


void TextTerminal::set_cursor_pos(core::Vec2u pos)
{
    // make sure new cursor position is not outside screen area
    m_cursor = {
        std::min(pos.x, m_cells.x),
        std::min(pos.y, m_cells.y),
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
    m_attrs.set_font_style(style);
}


void TextTerminal::set_decoration(Decoration decoration)
{
    m_attrs.set_decoration(decoration);
}


void TextTerminal::set_mode(Mode mode)
{
    m_attrs.set_mode(mode);
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


void TextTerminal::resize(View& view)
{
    auto& font = theme().font();
    m_font_size = view.size_to_framebuffer(m_font_size_requested);
    font.set_size(m_font_size.as<unsigned>());
    m_cell_size = view.size_to_viewport(FramebufferSize{
            font.max_advance(), font.height()});
    if (m_resize_cells) {
        m_cells = {(size().x / m_cell_size.x).as<unsigned>(),
                   (size().y / m_cell_size.y).as<unsigned>()};
    }

    m_frame.clear();
    m_frame.add_rectangle({{0, 0}, size()}, 0.01f);
    m_frame.update();
}


void TextTerminal::update(View& view, State state)
{
    view.finish_draw();

    auto& font = theme().font();
    font.set_size(m_font_size.as<unsigned>());

    size_t expected_num_cells = m_cells.x * m_cells.y / 2;
    m_sprites.clear();
    m_sprites.reserve(expected_num_cells);
    m_boxes.clear();
    m_boxes.reserve(0, expected_num_cells, 0);

    ViewportCoords pen;
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
        m_sprites.set_color(Color(7));
        m_boxes.set_fill_color(Color(0));

        class LineRenderer: public terminal::Renderer {
        public:
            // capture by ref
            LineRenderer(ViewportCoords& pen, size_t& column,
                    graphics::ColoredSprites& sprites, graphics::Shape& boxes,
                    text::Font& font,
                    const ViewportSize& cell_size, View& view)
                    : pen(pen), column(column), sprites(sprites), boxes(boxes),
                      font(font), ascender(font.ascender()),
                      cell_size(cell_size), view(view)
            {}

            void set_font_style(FontStyle font_style) override {
                font.set_style(font_style);
                ascender = font.ascender();
            }
            void set_decoration(Decoration decoration) override {
                //
            }
            void set_mode(Mode mode) override {
                m_mode = mode;
                switch (mode) {
                    default:
                    case Mode::Normal:
                        if (m_fg < 8)
                            sprites.set_color(Color(m_fg));
                        break;
                    case Mode::Bright:
                        if (m_fg < 8)
                            sprites.set_color(Color(m_fg + 8));
                        break;
                }
            }
            Color8bit _apply_fg8_mode() {
                return m_mode == Mode::Bright && m_fg < 8 ? m_fg + 8 : m_fg;
            }
            void set_default_fg_color() override {
                m_fg = 7;
                sprites.set_color(Color(m_mode == Mode::Bright ? 15 : 7));
            }
            void set_default_bg_color() override {
                m_bg = 0;
                boxes.set_fill_color(Color(0));
            }
            void set_fg_color(Color8bit fg) override {
                m_fg = fg;
                sprites.set_color(Color(m_mode == Mode::Bright && fg < 8 ? fg + 8 : fg));
            }
            void set_bg_color(Color8bit bg) override {
                m_bg = bg;
                boxes.set_fill_color(Color(bg));
            }
            void set_fg_color(Color24bit fg) override {
                m_fg = 255;
                sprites.set_color(fg);
            }
            void set_bg_color(Color24bit bg) override {
                m_bg = 255;
                boxes.set_fill_color(bg);
            }
            void draw_blanks(size_t num) override {
                boxes.add_rectangle({pen.x, pen.y, cell_size.x * num, cell_size.y});
                pen.x += cell_size.x * num;
                column += num;
            }
            void draw_char(CodePoint code_point) override {
                auto glyph = font.get_glyph(code_point);
                if (glyph == nullptr)
                    glyph = font.get_glyph(' ');

                auto bearing = view.size_to_viewport(FramebufferSize{glyph->bearing()});
                auto ascender_vp = view.size_to_viewport(FramebufferPixels{ascender});
                auto glyph_size = view.size_to_viewport(FramebufferSize{glyph->size()});
                sprites.add_sprite({
                        pen.x + bearing.x,
                        pen.y + (ascender_vp - bearing.y),
                        glyph_size.x,
                        glyph_size.y
                }, glyph->tex_coords());

                boxes.add_rectangle({pen, cell_size});

                pen.x += cell_size.x;
                ++column;
            }

        private:
            ViewportCoords& pen;
            size_t& column;
            graphics::ColoredSprites& sprites;
            graphics::Shape& boxes;
            text::Font& font;
            float ascender;
            Color8bit m_fg = 7;
            Color8bit m_bg = 0;
            Mode m_mode = Mode::Normal;
            const ViewportSize& cell_size;
            View& view;
        } line_renderer(pen, column, m_sprites, m_boxes, font,
                m_cell_size, view);

        line.render(line_renderer);

        // draw rest of blanked line
        if (line.is_blanked()) {
            ViewportRect rect {
                    pen.x, pen.y,
                    m_cell_size.x * (m_cells.x - column), m_cell_size.y };
            m_boxes.add_rectangle(rect);
        }

        // draw rest of blanked page
        if (line.is_page_blanked()) {
            ViewportRect rect {
                    0, pen.y + m_cell_size.y,
                    m_cell_size.x * m_cells.x, m_cell_size.y * (m_cells.y - row - 1) };
            m_boxes.add_rectangle(rect);
        }

        pen.x = 0;
        pen.y += m_cell_size.y;
    }
    m_boxes.update();
    m_sprites.update();

    m_caret.update(view, {m_cell_size.x * m_cursor.x, m_cell_size.y * m_cursor.y,
                          m_cell_size.x, m_cell_size.y});

    if (m_bell_time > 0ns) {
        m_bell_time -= state.elapsed;
        auto alpha = (float) duration_cast<milliseconds>(m_bell_time).count() / 500.f;
        m_frame.set_outline_color(Color(1.f, 0.f, 0.f, alpha));
        m_frame.update();
        // Request new draw and wakeup event loop immediately
        view.refresh();
        view.window()->wakeup();
    }
}


void TextTerminal::draw(View& view)
{
    m_boxes.draw(view, position());
    m_sprites.draw(view, position());
    m_caret.draw(view, position());

    if (m_bell_time > 0ns) {
        m_frame.draw(view, position());
        // Request new draw and wakeup event loop immediately
        view.refresh();
        view.window()->wakeup();
    }
}


} // namespace xci::widgets
