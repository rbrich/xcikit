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
#include <xci/graphics/Sprites.h>
#include <xci/graphics/Shape.h>
#include <xci/graphics/Primitives.h>
#include <xci/core/geometry.h>
#include <absl/strings/string_view.h>
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
// The same pattern applies for lower ctl codes too:
//  0-3  -> 0
//  4-7  -> 1
//  8-11 -> 2
// 12-15 -> 3
namespace ctl {
    static constexpr uint8_t blanks = 7;        // blanks (1b param - num of cells)

    static constexpr uint8_t first_introducer = 16;
    static constexpr uint8_t default_fg = 16;
    static constexpr uint8_t default_bg = 17;
    static constexpr uint8_t fg8bit = 20;
    static constexpr uint8_t bg8bit = 21;
    static constexpr uint8_t set_attrs = 22;
    static constexpr uint8_t fg24bit = 30;
    static constexpr uint8_t bg24bit = 31;
    static constexpr uint8_t last_introducer = 31;
}

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


class Renderer {
public:
    virtual void set_font_style(text::FontStyle font_style) = 0;
    virtual void set_fg_color(graphics::Color fg) = 0;
    virtual void set_bg_color(graphics::Color bg) = 0;
    virtual void draw_blanks(size_t num) = 0;
    virtual void draw_char(text::CodePoint code_point) = 0;
};


class Attributes {
public:
    // ------------------------------------------------------------------------
    // Encoding (internal format, not to be presented in public TextTerminal API).

    // Check character `c`, return true if it introduces or terminates
    // attribute sequence.
    static bool is_introducer(char c) { return c >= ctl::first_introducer && c <= ctl::last_introducer; }

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
    size_t decode(absl::string_view sv);

    // ------------------------------------------------------------------------
    // Mutators

    void set_fg(Color8bit fg_color);
    void set_bg(Color8bit bg_color);
    void set_fg(Color24bit fg_color);
    void set_bg(Color24bit bg_color);
    void set_default_fg();
    void set_default_bg();

    void set_italic(bool italic) { set_bit(Attr); m_attr[Italic] = italic; }
    void set_bold(bool bold) { set_bit(Attr); m_attr[Bold] = bold; }

    // Update this to go after `other` in stream,
    // ie. reset/skip all attributes set in `other`.
    void preceded_by(const Attributes& other);

    // ------------------------------------------------------------------------
    // Accessors

    bool has_attr() const { return m_set[Attr]; }
    text::FontStyle font_style() const { return text::FontStyle(m_attr.to_ulong() & c_font_style_mask); }

    bool has_fg() const { return m_set[Fg]; }
    bool has_bg() const { return m_set[Bg]; }
    graphics::Color fg() const;
    graphics::Color bg() const;

    void render(Renderer& renderer);

private:
    void set_bit(size_t i) { m_set.set(i, true); }

private:
    enum class ColorMode { ColorDefault, Color8bit, Color24bit };
    uint8_t m_fg_r, m_fg_g, m_fg_b;
    uint8_t m_bg_r, m_bg_g, m_bg_b;
    ColorMode m_fg = ColorMode::ColorDefault;
    ColorMode m_bg = ColorMode::ColorDefault;

    enum { Italic, Bold, _attr_count_ };
    std::bitset<_attr_count_> m_attr;

    enum { Attr, Fg, Bg, _flag_count_};
    std::bitset<_flag_count_> m_set;
};


class Line {
public:

    /// Clear the line and set initial attributes.
    void clear(const Attributes& attr);

    /// Skip `pos` characters, set `attr` for the following char(s)
    /// and insert `sv` or replace current cells at `pos` with content from `sv`.
    void add_text(size_t pos, absl::string_view sv, Attributes attr, bool insert);

    /// Delete part of line, shifting the rest to the left
    void delete_text(size_t first, size_t num);

    /// Erase part of line, replacing it with blank chars with `attr`
    void erase_text(size_t first, size_t num, Attributes attr);

    /// Mark the line with line break control code to forbid reflow.
    void set_hard_break() { m_flags[HardBreak] = true; }

    void set_blank_page() { m_flags[BlankPage] = true; }

    absl::string_view content() const { return m_content; }

    void render(Renderer& renderer);

    /// Skip `skip` chars, starting at `start`. Attributes at `start` are `attr`.
    /// \param skip     num of chars to skip
    /// \param start    index into content where to start
    /// \param attr     [INOUT] IN: attrs at start, OUT attrs at new pos
    /// \returns        new pos (ie. start + skip)
    size_t content_skip(size_t skip, size_t start, Attributes& attr);

    int length() const;

    bool is_blanked() const { return m_flags[BlankLine]; }
    bool is_page_blanked() const { return m_flags[BlankPage]; }

private:
    const char* content_begin() const { return m_content.c_str(); }
    const char* content_end() const { return m_content.c_str() + m_content.size(); }

private:
    std::string m_content;

    enum {
        HardBreak,    // hard line break
        BlankLine,    // blanked rest of line
        BlankPage,    // blanked rest of page
        _flag_count_
    };
    std::bitset<_flag_count_> m_flags;

    // TODO: possible optimization (benchmark!)
    // index cache - this avoids searching from begining for each operation
    // the index will usually be the same as cursor
    //size_t m_last_content_pos = 0;  // index into content where a char touched last time begins
    //size_t m_last_char_index = 0;  // char index (cell) touched last tiem
    //Attributes m_last_attr;
};


class Buffer {
public:
    Buffer() : m_lines(1) {}

    void add_line();
    void remove_lines(size_t start, size_t count);

    size_t size() const { return m_lines.size(); }

    Line& operator[] (int line_index) { return m_lines[line_index]; }

private:
    std::vector<Line> m_lines;
};


class Cursor {
public:
    explicit Cursor(graphics::Renderer& renderer = graphics::Renderer::default_renderer())
    : m_prim(renderer.create_primitives(graphics::VertexFormat::V2t2,
                                        graphics::PrimitiveType::TriFans)) {}

    void update(View& view, const core::Rect_f& rect);
    void draw(View& view, const Vec2f& pos);

private:
    void init_shader();

private:
    graphics::PrimitivesPtr m_prim;
    graphics::ShaderPtr m_shader;
};


} // terminal


class TextTerminal: public Widget {
public:

    TextTerminal() { set_focusable(true); }

    // ------------------------------------------------------------------------
    // Font size, number of cells

    /// Set font size and font scaling mode:
    /// \param size     size in screen pixels or scalable units (depending on `scalable` param)
    /// \param scalable false: size is in screen pixels, font size stays same when window resizes
    ///                 true: size in scalable units, number of cells stays (circa) the same when window resizes
    void set_font_size(float size, bool scalable);
    core::Vec2u size_in_cells() const { return m_cells; }

    // ------------------------------------------------------------------------
    // Text buffer

    /// Add text at current cursor position, moving the cursor to end of new text.
    /// \param insert   insert characters, shifting rest of the line (false = replace)
    /// \param wrap     wrap to next line if the cursor gets to right border of page
    ///                 (false = no wrap, keep overwriting the right-most cell)
    void add_text(absl::string_view text, bool insert=false, bool wrap=true);

    /// Forced line end (disallow reflow for current line).
    void break_line() { current_line().set_hard_break(); }
    void new_line();
    terminal::Line& current_line() { return (*m_buffer)[m_buffer_offset + m_cursor.y]; }

    /// Erase `num` chars from `first`, replacing them with current attr (blanks)
    /// \param first    first cell to be erased
    /// \param num      num cells to erase or 0 for rest of the line
    void erase_in_line(size_t first, size_t num);
    /// Erase from cursor to the end of page
    void erase_to_end_of_page();
    /// Erase from the beginning of the page
    /// up to and including the cursor position
    void erase_to_cursor();
    /// Erase page (visible part of the buffer)
    void erase_page();
    /// Erase whole buffer (page + scrollback)
    void erase_buffer();
    /// Erase scrollback buffer (the scrolled-out part)
    void erase_scrollback();

    /// Set character buffer to `new_buffer`, returning current buffer.
    /// \param new_buffer   unique_ptr to Buffer object (must be initialized)
    /// Example usage:
    ///     auto new_buffer = std::make_unique<terminal::Buffer>();
    ///     auto old_buffer = terminal.set_buffer(std::move(new_buffer));
    std::unique_ptr<terminal::Buffer> set_buffer(std::unique_ptr<terminal::Buffer> new_buffer);

    // ------------------------------------------------------------------------
    // Cursor positioning

    void set_cursor_pos(core::Vec2u pos);
    core::Vec2u cursor_pos() const { return m_cursor; }

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

    void set_default_fg() { m_attrs.set_default_fg(); }
    void set_default_bg() { m_attrs.set_default_bg(); }

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
    // Set/Reset all attributes

    void set_attrs(terminal::Attributes attrs) { m_attrs = attrs; }
    void reset_attrs() { m_attrs = {}; }
    terminal::Attributes attrs() const { return m_attrs; }

    // ------------------------------------------------------------------------
    // Effects

    // visual bell
    void bell();

    // mouse scroll (scrollback)
    void scrollback(double lines);
    void cancel_scrollback();

    // ------------------------------------------------------------------------
    // impl Widget

    void update(View& view, std::chrono::nanoseconds elapsed) override;
    void resize(View& view) override;
    void draw(View& view, State state) override;

private:
    static constexpr double c_scroll_end = std::numeric_limits<double>::infinity();

    float m_font_size = 14.0;
    bool m_font_scalable = false;
    core::Vec2f m_cell_size;
    core::Vec2u m_cells = {80, 25};  // rows, columns
    std::unique_ptr<terminal::Buffer> m_buffer = std::make_unique<terminal::Buffer>();
    size_t m_buffer_offset = 0;  // offset to line in buffer which is first on page
    double m_scroll_offset = c_scroll_end;  // scroll back with mouse wheel
    core::Vec2u m_cursor;  // x/y of cursor on screen
    terminal::Attributes m_attrs;  // current attributes
    std::chrono::nanoseconds m_bell_time {0};
};


}} // namespace xci::widgets


#endif // XCI_WIDGETS_TEXTTERMINAL_H
