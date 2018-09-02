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
#include <xci/util/log.h>

#ifdef XCI_EMBED_SHADERS
#define INCBIN_PREFIX g_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include <incbin/incbin.h>
INCBIN(cursor_vert, XCI_SHARE_DIR "/shaders/cursor.vert");
INCBIN(cursor_frag, XCI_SHARE_DIR "/shaders/cursor.frag");
#endif

namespace xci {
namespace widgets {

using namespace graphics;
using namespace util;
using namespace util::log;
using namespace std::chrono;
using namespace std::chrono_literals;
using std::min;
using text::CodePoint;


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
        result.push_back(ctl::set_attrs);
        result.push_back(char(m_attr.to_ulong()));
    }

    if (m_set[Fg]) {
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

    if (m_set[Bg]) {
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


size_t terminal::Attributes::decode(std::string_view sv)
{
    auto* it = sv.cbegin();
    while (it < sv.cend()) {
        if (*it < ctl::first_introducer || *it > ctl::last_introducer)
            break;
        switch (*it) {
            case ctl::set_attrs:
                m_set[Attr] = true;
                m_attr = (*++it);
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
                set_bit(Fg);
                m_fg = ColorMode::Color24bit;
                m_fg_r = uint8_t(*++it);
                m_fg_g = uint8_t(*++it);
                m_fg_b = uint8_t(*++it);
                break;
            case ctl::bg24bit:
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
        default:
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


void terminal::Attributes::render(terminal::Renderer& renderer)
{
    if (has_attr()) {
        renderer.set_font_style(font_style());
    }
    if (has_fg())
        renderer.set_fg_color(fg());
    if (has_bg())
        renderer.set_bg_color(bg());
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
        if (Attributes::is_introducer(m_content[pos]))
            pos += attr.decode({content_begin() + pos, m_content.size() - pos});
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
        pos = utf8_next(content_begin() + pos) - content_begin();
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


void terminal::Line::add_text(size_t pos, std::string_view sv, Attributes attr, bool insert)
{
    // Find `pos` in content
    Attributes attr_start;
    auto start = content_skip(pos, 0, attr_start);

    // Now we are at `pos` (or content end), but there might be some attribute
    Attributes attr_end(attr_start);
    auto end = start;
    if (Attributes::is_introducer(m_content[end]))
        end += attr_end.decode({content_begin() + end, m_content.size() - end});

    // Replace mode - find end of the place for new text (same length as `sv`)
    if (!insert) {
        auto len = utf8_length(sv);
        end = content_skip(len, end, attr_end);

        // Read also attributes after replace span
        // and unify them with attr_end
        if (Attributes::is_introducer(m_content[end]))
            end += attr_end.decode({content_begin() + end, m_content.size() - end});
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
    if (Attributes::is_introducer(m_content[end]))
        end += attr_end.decode({content_begin() + end, m_content.size() - end});

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
    if (Attributes::is_introducer(m_content[end]))
        end += attr_end.decode({content_begin() + end, m_content.size() - end});

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
    for (const char* it = content_begin(); it != content_end(); ) {
        if (*it == terminal::ctl::blanks) {
            auto num = uint8_t(*++it);
            renderer.draw_blanks(num);
            ++it;
            continue;
        }
        if (terminal::Attributes::is_introducer(*it)) {
            terminal::Attributes attr;
            it += attr.decode({it, size_t(content_end() - it)});
            attr.render(renderer);
            continue;
        }

        // extract single UTF-8 character
        renderer.draw_char(utf8_codepoint(it));
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


void terminal::Cursor::update(View& view, const Rect_f& rect)
{
    float x1 = rect.x;
    float y1 = rect.y;
    float x2 = rect.x + rect.w;
    float y2 = rect.y + rect.h;
    float tx = 2.0f * view.screen_ratio().x / rect.w;
    float ty = 2.0f * view.screen_ratio().y / rect.h;
    float ix = 1.0f + tx / (1-tx);
    float iy = 1.0f + ty / (1-ty);

    m_prim->clear();
    m_prim->begin_primitive();
    m_prim->add_vertex(x1, y1, -ix, -iy);
    m_prim->add_vertex(x1, y2, -ix, +iy);
    m_prim->add_vertex(x2, y2, +ix, +iy);
    m_prim->add_vertex(x2, y1, +ix, -iy);
    m_prim->end_primitive();
}


void terminal::Cursor::draw(View& view, const Vec2f& pos)
{
    // pure white
    constexpr Color fill_color(0.7, 0.7, 0.7);
    constexpr Color outline_color(0.7, 0.7, 0.7);
    // green
    //constexpr Color fill_color(0.0, 0.7, 0.3);
    //constexpr Color outline_color(0.0, 1.0, 0.0);
    // yellow
    //constexpr Color fill_color(0.7, 0.7, 0.3);
    //constexpr Color outline_color(1.0, 1.0, 0.0);
    init_shader();
    m_prim->set_shader(m_shader);
    m_prim->set_uniform("u_fill_color",
                        fill_color.red_f(), fill_color.green_f(),
                        fill_color.blue_f(), fill_color.alpha_f());
    m_prim->set_uniform("u_outline_color",
                        outline_color.red_f(), outline_color.green_f(),
                        outline_color.blue_f(), outline_color.alpha_f());
    m_prim->set_blend(Primitives::BlendFunc::InverseVideo);
    m_prim->draw(view, pos);
}


void terminal::Cursor::init_shader()
{
    if (m_shader)
        return;
    auto& renderer = graphics::Renderer::default_renderer();
    m_shader = renderer.new_shader(ShaderId::Cursor);

#ifdef XCI_EMBED_SHADERS
    bool res = m_shader->load_from_memory(
                (const char*)g_cursor_vert_data, g_cursor_vert_size,
                (const char*)g_cursor_frag_data, g_cursor_frag_size);
)
#else
    bool res = m_shader->load_from_vfs("shaders/cursor.vert", "shaders/cursor.frag");
#endif
    if (!res) {
        log_error("Cursor shader not loaded!");
    }
}


// ------------------------------------------------------------------------


void TextTerminal::set_font_size(float size, bool scalable)
{
    m_font_size = size;
    m_font_scalable = scalable;
}


void TextTerminal::add_text(std::string_view text, bool insert, bool wrap)
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


void TextTerminal::update(View& view, std::chrono::nanoseconds elapsed)
{
    if (m_bell_time > 0ns) {
        m_bell_time -= elapsed;
        // Request new draw and wakeup event loop immediately
        view.refresh();
        view.window()->wakeup();
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

    size_t expected_num_cells = m_cells.x * m_cells.y / 2;
    sprites.reserve(expected_num_cells);
    boxes.reserve(0, expected_num_cells, 0);

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
        auto ascender = font.ascender();

        class LineRenderer: public terminal::Renderer {
        public:
            // capture by ref
            LineRenderer(Vec2f& pen, size_t& column,
                         graphics::ColoredSprites& sprites, graphics::Shape& boxes,
                         text::Font& font, float& ascender,
                         const util::Vec2f& cell_size, const Vec2f& pxf)
            : pen(pen), column(column), sprites(sprites), boxes(boxes),
              font(font), ascender(ascender),
              cell_size(cell_size), pxf(pxf)
            {}

            void set_font_style(FontStyle font_style) override {
                font.set_style(font_style);
                ascender = font.ascender();
                //auto deco = static_cast<Decoration>((*it & c_decoration_mask) >> c_decoration_shift);
                //auto mode = static_cast<Mode>((*it & c_mode_mask) >> c_mode_shift);
                //(void) deco; // TODO
                //(void) mode; // TODO
            }
            void set_fg_color(Color fg) override {
                sprites.set_color(fg);
            }
            void set_bg_color(Color bg) override {
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

                sprites.add_sprite({
                        pen.x + glyph->base_x() * pxf.x,
                        pen.y + (ascender - glyph->base_y()) * pxf.y,
                        glyph->width() * pxf.x,
                        glyph->height() * pxf.y
                    }, glyph->tex_coords());

                boxes.add_rectangle({pen, cell_size});

                pen.x += cell_size.x;
                ++column;
            }

        private:
            Vec2f& pen;
            size_t& column;
            graphics::ColoredSprites& sprites;
            graphics::Shape& boxes;
            text::Font& font;
            float& ascender;
            const util::Vec2f& cell_size;
            const util::Vec2f& pxf;
        } line_renderer(pen, column, sprites, boxes, font, ascender,
                        m_cell_size, pxf);

        line.render(line_renderer);

        // draw rest of blanked line
        if (line.is_blanked()) {
            Rect_f rect { pen.x, pen.y, m_cell_size.x * (m_cells.x - column), m_cell_size.y };
            boxes.add_rectangle(rect);
        }

        // draw rest of blanked page
        if (line.is_page_blanked()) {
            Rect_f rect { 0, pen.y + m_cell_size.y,
                          m_cell_size.x * m_cells.x, m_cell_size.y * (m_cells.y - row - 1) };
            boxes.add_rectangle(rect);
        }

        pen.x = 0;
        pen.y += m_cell_size.y;
    }

    boxes.draw(view, position());
    sprites.draw(view, position());

    // draw the cursor
    terminal::Cursor cursor;
    cursor.update(view, {m_cell_size.x * m_cursor.x, m_cell_size.y * m_cursor.y,
                         m_cell_size.x, m_cell_size.y});
    cursor.draw(view, position());

    if (m_bell_time > 0ns) {
        auto x = (float) duration_cast<milliseconds>(m_bell_time).count() / 500.f;
        graphics::Shape frame(Color::Transparent(), Color(1.f, 0.f, 0.f, x));
        frame.add_rectangle({{0, 0}, size()}, 0.01f);
        frame.draw(view, position());
        // Request new draw and wakeup event loop immediately
        view.refresh();
        view.window()->wakeup();
    }
}


}} // namespace xci::widgets
