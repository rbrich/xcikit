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
#include <bitset>

namespace xci {
namespace widgets {


// Gory bits...
namespace terminal {


// Internal control codes
// ----------------------
// Codes 16 to 31 are reserved for attribute introducers
// Number of parameters are deducible from introducer value:
// 16-19 -> 0
// 20-23 -> 1
// 24-27 -> 2
// 28-31 -> 3
static constexpr uint8_t c_ctl_new_line = 10;
static constexpr uint8_t c_ctl_first_introducer = 16;
static constexpr uint8_t c_ctl_last_introducer = 31;
static constexpr uint8_t c_ctl_set_attrs = 22;
static constexpr uint8_t c_ctl_reset_attrs = 23;
static constexpr uint8_t c_ctl_default_fg = 16;
static constexpr uint8_t c_ctl_default_bg = 17;
static constexpr uint8_t c_ctl_fg8bit = 20;
static constexpr uint8_t c_ctl_bg8bit = 21;
static constexpr uint8_t c_ctl_fg24bit = 30;
static constexpr uint8_t c_ctl_bg24bit = 31;

// Encoding of attributes byte
static constexpr uint8_t c_font_style_mask = 0b00000011;
static constexpr uint8_t c_decoration_mask = 0b00011100;
static constexpr uint8_t c_mode_mask       = 0b01100000;
static constexpr int c_decoration_shift = 2;
static constexpr int c_mode_shift = 5;


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

    // Check character `c`, return true if it introduces or terminates
    // attribute sequence.
    static bool is_introducer(char c) { return c >= c_ctl_first_introducer && c <= c_ctl_last_introducer; }

    // Skip over attribute sequence.
    // Return pointer to following non-attr character.
    // Expects that `is_introducer(*first) = true`.
    static const char* skip(const char* introducer);

    static size_t length(const char* introducer) { return skip(introducer) - introducer; }

    /// Encode object state into sequence of attribute bytes.
    /// Max length of encoded string is 13:
    /// 2*4 for colors (24bit) + 2*2 to set & reset attrs + 1 for terminator
    std::string encode() const;

    /// Decode attribute sequence from string_view into object state
    /// This is usable for incremental (running) decoder.
    size_t decode(std::string_view sv);

    // ------------------------------------------------------------------------
    // Mutators

    void set_fg(Color8bit fg_color);
    void set_bg(Color8bit bg_color);
    void set_fg(Color24bit fg_color);
    void set_bg(Color24bit bg_color);
    void reset_fg() { reset_bit(Fg); }
    void reset_bg() { reset_bit(Bg); }

    void set_italic(bool italic) { set_bit(Italic, italic); }
    void set_bold(bool bold) { set_bit(Bold, bold); }

    // Combine attributes from `other` into this.
    void include_from(const Attributes& other);
    // Remove attributes also in `other` from this.
    void exclude_from(const Attributes& other);

    // ------------------------------------------------------------------------
    // Accessors

    text::FontStyle font_style() const { return text::FontStyle(m_set.to_ulong() & c_font_style_mask); }

    bool has_fg() const { return m_set[Fg]; }
    bool has_bg() const { return m_set[Bg]; }
    graphics::Color fg() const;
    graphics::Color bg() const;

private:
    void set_bit(size_t i) { set_bit(i, true); }
    void reset_bit(size_t i) { set_bit(i, false); }
    void set_bit(size_t i, bool value) { m_set[i] = value; m_reset[i] = !value; }

private:
    uint8_t m_fg_r, m_fg_g, m_fg_b;
    bool m_fg8bit;
    uint8_t m_bg_r, m_bg_g, m_bg_b;
    bool m_bg8bit;

    enum { Italic, Bold, Fg, Bg, _flag_count_};
    std::bitset<_flag_count_> m_set;
    std::bitset<_flag_count_> m_reset;

    static constexpr uint8_t c_attrs_mask = 0b11;
};


class Line {
public:

    /// Clear the line and set initial attributes.
    void clear(const Attributes& attr);

    /// Skip `pos` characters, set `attr` for the following char(s)
    /// and insert `sv` or replace current cells at `pos` with content from `sv`.
    void add_text(size_t pos, std::string_view sv, Attributes attr, bool insert);

    void erase_text(int first, int num);

    /// Mark the line with line break control code to forbid reflow.
    void set_line_break();

    std::string_view content() const { return m_content; }

    const char* content_begin() const { return m_content.c_str(); }
    const char* content_end() const { return m_content.c_str() + m_content.size(); }
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

    void add_text(std::string_view text, bool insert=false);

    // Forced line end (disallow reflow for current line).
    void break_line() { current_line().set_line_break(); }
    void new_line();
    terminal::Line& current_line() { return m_buffer[m_buffer_offset + m_cursor.y]; }

    // Erase page (visible part of the buffer)
    void erase_page();
    // Erase whole buffer (page + scrollback)
    void erase_buffer();

    // ------------------------------------------------------------------------
    // Cursor positioning

    void set_cursor_pos(util::Vec2u pos);
    util::Vec2u cursor_pos() const { return m_cursor; }
    util::Vec2u size_in_cells() const { return m_cells; }

    // ------------------------------------------------------------------------
    // Color attributes

    using Color4bit = terminal::Color4bit;
    using Color8bit = terminal::Color8bit;
    using Color24bit = terminal::Color24bit;

    // Basic 16 colors
    void set_fg(Color4bit fg_color) { set_fg(static_cast<Color8bit>(fg_color)); }
    void set_bg(Color4bit bg_color) { set_bg(static_cast<Color8bit>(bg_color)); }

    // Default (xterm-compatible) 256 color palette
    void set_fg(Color8bit fg_color) { m_attrs.set_fg(fg_color); }
    void set_bg(Color8bit bg_color) { m_attrs.set_bg(bg_color); }

    // "True color"
    void set_fg(Color24bit fg_color) { m_attrs.set_fg(fg_color); }
    void set_bg(Color24bit bg_color) { m_attrs.set_bg(bg_color); }

    void set_default_fg() { m_attrs.reset_fg(); }
    void set_default_bg() { m_attrs.reset_bg(); }

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

    // mouse scroll (scrollback)
    void scrollback(double lines);
    void cancel_scrollback();

    // ------------------------------------------------------------------------
    // impl Widget

    void update(std::chrono::nanoseconds elapsed) override;
    void resize(View& view) override;
    void draw(View& view, State state) override;

private:
    static constexpr double c_scroll_end = std::numeric_limits<double>::infinity();

    float m_font_size = 0.05;
    util::Vec2f m_cell_size;
    util::Vec2u m_cells = {80, 25};  // rows, columns
    terminal::Buffer m_buffer;
    size_t m_buffer_offset = 0;  // offset to line in buffer which is first on page
    double m_scroll_offset = c_scroll_end;  // scroll back with mouse wheel
    util::Vec2u m_cursor;  // x/y of cursor on screen
    terminal::Attributes m_attrs;  // current attributes
    std::chrono::nanoseconds m_bell_time {0};
};


}} // namespace xci::widgets


#endif // XCI_WIDGETS_TEXTTERMINAL_H
