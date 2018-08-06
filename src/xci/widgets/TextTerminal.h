// TextTerminal.h created on 2018-07-19, part of XCI toolkit
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

#ifndef XCI_WIDGETS_TEXTTERMINAL_H
#define XCI_WIDGETS_TEXTTERMINAL_H

#include <xci/widgets/Widget.h>
#include <xci/text/FontFace.h>
#include <xci/util/geometry.h>
#include <xci/compat/string_view.h>
#include <vector>
#include <chrono>

namespace xci {
namespace widgets {


namespace terminal {


enum class Color4bit {
    Black, Red, Green, Yellow, Blue, Magenta, Cyan, White,
    BrightBlack, BrightRed, BrightGreen, BrightYellow,
    BrightBlue, BrightMagenta, BrightCyan, BrightWhite
};
using Color8bit = uint8_t;
using Color24bit = graphics::Color;  // alpha channel is ignored


class Attributes {
public:
    // ------------------------------------------------------------------------
    // Encoding (internal format, not to be presented in public TextTerminal API).

    // Check character `c`, return true if it introduces attribute sequence.
    static bool is_introducer(char c);

    // Having string_view with first char introducing attribute sequence,
    // return length of the sequence.
    static size_t encoded_length(std::string_view sv);

    // Decode attribute sequence from string_view into object state
    void decode(std::string_view sv);

    // Encode object state into attribute sequence
    const std::string& encode() const { return m_encoded_attrs; }

    // ------------------------------------------------------------------------
    // Mutators

    void set_fg(Color8bit fg_color);
    void reset_fg();  // set to default value

    // ------------------------------------------------------------------------
    // Accessors




private:
    std::string m_encoded_attrs;
};


class Line {
public:
    void set_attr(int pos, const Attributes& attr);
    Attributes get_attr(int pos);

    void insert_text(int pos, std::string_view sv);
    void append_text(std::string_view string) { m_content.append(string.cbegin(), string.cend()); }
    void replace_text(int pos, std::string_view sv);
    void erase_text(int first, int num);

    const std::string& content() const { return m_content; }
    int length() const;

private:
    std::string m_content;
};


class Buffer {
public:
    Buffer() : m_lines(1) {}

    void add_line() { m_lines.emplace_back(); }
    void remove_lines(size_t start, size_t count);

    size_t size() const { return m_lines.size(); }

    Line& operator[] (int line_index) { return m_lines[line_index]; }

private:
    std::vector<Line> m_lines;
};


} // terminal


class TextTerminal: public Widget {
public:
    // ------------------------------------------------------------------------
    // Text content

    void add_text(std::string_view text);
    void new_line();
    terminal::Line& current_line() { return m_buffer[m_buffer_offset + m_cursor.y]; }

    // Erase page (visible part of the buffer)
    void erase_page();
    // Erase whole buffer (page + scrollback)
    void erase_buffer();

    // ------------------------------------------------------------------------
    // Cursor positioning

    void set_cursor_pos(util::Vec2i pos);
    util::Vec2i cursor_pos() const { return m_cursor; }
    util::Vec2i size_in_cells() const { return m_cells; }

    // ------------------------------------------------------------------------
    // Color attributes

    using Color4bit = terminal::Color4bit;
    using Color8bit = terminal::Color8bit;
    using Color24bit = terminal::Color24bit;

    // Basic 16 colors
    void set_fg(Color4bit fg_color) { set_fg(static_cast<Color8bit>(fg_color)); }
    void set_bg(Color4bit bg_color) { set_bg(static_cast<Color8bit>(bg_color)); }

    // Default (xterm-compatible) 256 color palette
    void set_fg(Color8bit fg_color);
    void set_bg(Color8bit bg_color);

    // "True color"
    void set_fg(Color24bit fg_color);
    void set_bg(Color24bit bg_color);

    // ------------------------------------------------------------------------
    // Text attributes

    using FontStyle = text::FontStyle;
    void set_font_style(FontStyle style);

    enum class Decoration {
        None,
        Underlined,
        Overlined,
        CrossedOut,
        Framed,
        Encircled,
    };
    void set_decoration(Decoration decoration);

    enum class Mode {
        Normal,
        Blink,
        Conceal,
        Reverse,
    };
    void set_mode(Mode mode);

    // ------------------------------------------------------------------------
    // Effects

    // visual bell
    void bell();

    // ------------------------------------------------------------------------
    // impl Widget

    void update(std::chrono::nanoseconds elapsed) override;
    void resize(View& view) override;
    void draw(View& view, State state) override;

private:
    void set_attribute(uint8_t mask, uint8_t attr);

private:
    float m_font_size = 0.05;
    util::Vec2f m_cell_size;
    util::Vec2i m_cells = {80, 25};  // rows, columns
    terminal::Buffer m_buffer;
    int m_buffer_offset = 0;  // offset to line in buffer which is first on page
    util::Vec2i m_cursor;  // x/y of cursor on screen
    uint8_t m_attributes = 0;  // encoded attributes (font style, mode, decorations)
    std::chrono::nanoseconds m_bell_time {0};
};


}} // namespace xci::widgets


#endif // XCI_WIDGETS_TEXTTERMINAL_H
