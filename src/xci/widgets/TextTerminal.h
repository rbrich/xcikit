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
#include <vector>
#include <chrono>
#include <xci/compat/string_view.h>

namespace xci {
namespace widgets {


namespace terminal {

class Line {
public:
    void insert(int pos, std::string_view string);
    void append(std::string_view string) { m_content.append(string.cbegin(), string.cend()); }
    void replace(int pos, std::string_view string);
    void erase(int first, int num);

    const std::string& content() const { return m_content; }
    int length() const;

private:
    std::string m_content;
};

class Buffer {
public:
    Buffer() : m_lines(1) {}

    void add_line() { m_lines.emplace_back(); }

    size_t size() const { return m_lines.size(); }

    Line& operator[] (int line_index) { return m_lines[line_index]; }

private:
    std::vector<Line> m_lines;
};

} // terminal


class TextTerminal: public Widget {
public:
    void add_text(const std::string& text);

    enum class Color4bit {
        Black, Red, Green, Yellow, Blue, Magenta, Cyan, White,
        BrightBlack, BrightRed, BrightGreen, BrightYellow,
        BrightBlue, BrightMagenta, BrightCyan, BrightWhite
    };
    void set_color(Color4bit fg, Color4bit bg);

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

    // visual bell
    void bell();

    void set_cursor_pos(util::Vec2i pos);
    util::Vec2i cursor_pos() const { return m_cursor; }

    terminal::Line& current_line() { return m_buffer[m_buffer_offset + m_cursor.y]; }
    util::Vec2i size_in_cells() const { return m_cells; }

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
    int m_buffer_offset = 0;  // offset to line in buffer which is first on screen
    util::Vec2i m_cursor;  // x/y of cursor on screen
    uint8_t m_attributes = 0;  // encoded attributes (font style, mode, decorations)
    std::chrono::nanoseconds m_bell_time {0};
};


}} // namespace xci::widgets


#endif // XCI_WIDGETS_TEXTTERMINAL_H
