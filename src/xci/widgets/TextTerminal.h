// TextTerminal.h created on 2018-07-19 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_WIDGETS_TEXTTERMINAL_H
#define XCI_WIDGETS_TEXTTERMINAL_H

#include <xci/widgets/Widget.h>
#include <xci/text/FontFace.h>
#include <xci/graphics/Sprites.h>
#include <xci/graphics/Shape.h>
#include <xci/graphics/Primitives.h>
#include <xci/graphics/View.h>
#include <xci/graphics/Shader.h>
#include <xci/core/geometry.h>
#include <string_view>
#include <vector>
#include <chrono>
#include <bitset>

namespace xci::widgets {

using graphics::FramebufferPixels;


// Gory bits...
namespace terminal {


// Internal control codes
// ----------------------
// Codes 16 to 31 are reserved for attribute introducers
// Number of parameters are deducible from introducer value:
// 16-19 (0x10-0x13) -> 0
// 20-23 (0x14-0x17)-> 1
// 24-27 (0x18-0x1b)-> 1
// 28-31 (0x1c-0x1f)-> 3
// The same pattern applies for lower ctl codes too:
//  0-3  -> 0
//  4-7  -> 1
//  8-11 -> 2
// 12-15 -> 3
namespace ctl {
    static constexpr uint8_t blanks = 7;        // blanks (1b param - num of cells)

    static constexpr uint8_t default_fg = 0x10;
    static constexpr uint8_t default_bg = 0x11;
    static constexpr uint8_t fg8bit = 0x14;
    static constexpr uint8_t bg8bit = 0x15;
    static constexpr uint8_t font_style = 0x18;
    static constexpr uint8_t decoration = 0x19;
    static constexpr uint8_t mode = 0x1a;
    static constexpr uint8_t fg24bit = 0x1e;
    static constexpr uint8_t bg24bit = 0x1f;

    static constexpr uint8_t first_introducer = 0x10;
    static constexpr uint8_t last_introducer = 0x1f;
} // namespace ctl


enum class Color4bit : uint8_t {
    Black, Red, Green, Yellow, Blue, Magenta, Cyan, White,
    BrightBlack, BrightRed, BrightGreen, BrightYellow,
    BrightBlue, BrightMagenta, BrightCyan, BrightWhite
};
using Color8bit = uint8_t;
using Color24bit = graphics::Color;  // alpha channel is ignored


enum class FontStyle {
    Regular,        // 000
    Italic,         // 001
    Bold,           // 010
    BoldItalic,     // 011
    Light,          // 100
    LightItalic,    // 101
};


enum class Decoration {
    None,
    Underlined,
    Overlined,
    CrossedOut,
    //Framed,
    //Encircled,
};


enum class Mode {
    Normal,
    Bright,
    //Blink,
    //Conceal,
    //Reverse,
};


class Renderer {
public:
    virtual void set_font_style(FontStyle font_style) = 0;
    virtual void set_decoration(Decoration decoration) = 0;
    virtual void set_mode(Mode mode) = 0;
    virtual void set_default_fg_color() = 0;
    virtual void set_default_bg_color() = 0;
    virtual void set_fg_color(Color8bit fg) = 0;
    virtual void set_bg_color(Color8bit bg) = 0;
    virtual void set_fg_color(Color24bit fg) = 0;
    virtual void set_bg_color(Color24bit bg) = 0;
    virtual void draw_blanks(size_t num) = 0;
    virtual void draw_chars(std::string_view utf8) = 0;
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
    /// \returns Number of bytes consumed
    size_t decode(std::string_view sv);

    // ------------------------------------------------------------------------
    // Mutators

    void set_fg(Color8bit fg_color);
    void set_bg(Color8bit bg_color);
    void set_fg(Color24bit fg_color);
    void set_bg(Color24bit bg_color);
    void set_default_fg();
    void set_default_bg();

    void set_font_style(terminal::FontStyle style);
    void set_mode(Mode mode);
    void set_decoration(Decoration decoration);

    // Update this to go after `other` in stream,
    // ie. reset/skip all attributes set in `other`.
    void preceded_by(const Attributes& other);

    // ------------------------------------------------------------------------
    // Accessors

    bool has_font_style() const { return m_set[FlagFontStyle]; }
    bool has_decoration() const { return m_set[FlagDecoration]; }
    bool has_mode() const { return m_set[FlagMode]; }
    FontStyle font_style() const { return has_font_style() ? m_font_style : FontStyle::Regular; }
    Mode mode() const { return has_mode() ? m_mode : Mode::Normal; }
    Decoration decoration() const { return has_decoration() ? m_decoration : Decoration::None; }

    bool has_fg() const { return m_set[FlagFg]; }
    bool has_bg() const { return m_set[FlagBg]; }
    graphics::Color fg() const;
    graphics::Color bg() const;

    void render(Renderer& renderer);

private:
    void set_bit(size_t i) { m_set.set(i, true); }

private:
    enum class ColorMode: uint8_t { ColorDefault, Color8bit, Color24bit };
    uint8_t m_fg_r, m_fg_g, m_fg_b;
    uint8_t m_bg_r, m_bg_g, m_bg_b;
    ColorMode m_fg = ColorMode::ColorDefault;
    ColorMode m_bg = ColorMode::ColorDefault;

    FontStyle m_font_style = FontStyle::Regular;
    Mode m_mode = Mode::Normal;
    Decoration m_decoration = Decoration::None;

    enum { FlagFontStyle, FlagDecoration, FlagMode, FlagFg, FlagBg, _flag_count_};
    std::bitset<_flag_count_> m_set;
};


class Line {
public:
    Line() { m_content.reserve(100); }

    /// Clear the line and set initial attributes.
    void clear(const Attributes& attr);

    /// Skip `pos` characters, set `attr` for the following char(s)
    /// and insert `sv` or replace current cells at `pos` with content from `sv`.
    void add_text(size_t pos, std::string_view sv, Attributes attr, bool insert);

    /// Delete part of line, shifting the rest to the left
    void delete_text(size_t first, size_t num);

    /// Erase part of line, replacing it with blank chars with `attr`
    void erase_text(size_t first, size_t num, Attributes attr);

    /// Mark the line with line break control code to forbid reflow.
    void set_hard_break() { m_flags[HardBreak] = true; }

    void set_blank_page() { m_flags[BlankPage] = true; }

    std::string_view content() const { return m_content; }

    void render(Renderer& renderer);

    /// Skip `skip` chars, starting at `start`. Attributes at `start` are `attr`.
    /// \param skip     num of chars to skip
    /// \param start    index into content where to start
    /// \param attr     [INOUT] IN: attrs at start, OUT attrs at new pos
    /// \returns        new pos (ie. start + skip)
    size_t content_skip(size_t skip, size_t start, Attributes& attr);

    size_t length() const;

    bool is_blanked() const { return m_flags[BlankLine]; }
    bool is_page_blanked() const { return m_flags[BlankPage]; }

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
    Buffer() : m_lines(1) { m_lines.reserve(100); }

    void add_line();
    void remove_lines(size_t start, size_t count);

    size_t size() const { return m_lines.size(); }

    Line& operator[] (size_t line_index) { return m_lines[line_index]; }

private:
    std::vector<Line> m_lines;
};


class Caret {
public:
    explicit Caret(graphics::Renderer& renderer)
    : m_quad(renderer, graphics::VertexFormat::V2t2,
             graphics::PrimitiveType::TriFans),
      m_shader(renderer.get_shader(graphics::ShaderId::Cursor)) {}

    void update(View& view, const ViewportRect& rect);
    void draw(View& view, const ViewportCoords& pos);

private:
    graphics::Primitives m_quad;
    graphics::Shader& m_shader;
};


} // namespace terminal


class TextTerminal: public Widget {
public:

    explicit TextTerminal(Theme& theme);

    // ------------------------------------------------------------------------
    // Font size, number of cells

    /// Set font size and font scaling mode:
    /// \param size     size in viewport units
    void set_font_size(ViewportUnits size);

    /// Set requested terminal size in cells (i.e. do not scale the number of cells
    /// according to widget size - keep it fixed)
    void set_size_in_cells(core::Vec2u cells) { m_cells = cells; m_resize_cells = false; }
    void reset_req_cells() { m_resize_cells = true; }

    /// Retrieve current (actual) terminal size
    core::Vec2u size_in_cells() const { return m_cells; }

    // ------------------------------------------------------------------------
    // Text buffer

    /// Add text at current cursor position, moving the cursor to end of new text.
    /// \param insert   insert characters, shifting rest of the line (false = replace)
    /// \param wrap     wrap to next line if the cursor gets to right border of page
    ///                 (false = no wrap, keep overwriting the right-most cell)
    void add_text(std::string_view text, bool insert=false, bool wrap=true);

    /// Forced line end (disallow reflow for current line).
    void break_line() { current_line().set_hard_break(); }
    void new_line();

    /// Get contents of Nth line of current page
    terminal::Line& line(size_t n) { return (*m_buffer)[m_buffer_offset + n]; }
    /// Get contents of current line (where cursor is)
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

    /// Cursor position is 0-based
    void set_cursor_pos(core::Vec2u pos) { set_cursor_x(pos.x); set_cursor_y(pos.y); }
    void set_cursor_x(uint32_t x) { m_cursor.x = std::min(x, m_cells.x); }
    void set_cursor_y(uint32_t y);
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

    using FontStyle = terminal::FontStyle;
    void set_font_style(FontStyle style);

    using Decoration = terminal::Decoration;
    void set_decoration(Decoration decoration);

    using Mode = terminal::Mode;
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

    void update(View& view, State state) override;
    void resize(View& view) override;
    void draw(View& view) override;

private:
    static constexpr double c_scroll_end = std::numeric_limits<double>::infinity();

    FramebufferPixels m_font_size = 0;
    ViewportUnits m_font_size_requested {14.0};
    ViewportSize m_cell_size;
    core::Vec2u m_cells = {80, 25};  // rows, columns
    bool m_resize_cells = true;
    std::unique_ptr<terminal::Buffer> m_buffer = std::make_unique<terminal::Buffer>();
    size_t m_buffer_offset = 0;  // offset to line in buffer which is first on page
    double m_scroll_offset = c_scroll_end;  // scroll back with mouse wheel
    core::Vec2u m_cursor;  // x/y of cursor on screen
    terminal::Attributes m_attrs;  // current attributes
    std::chrono::nanoseconds m_bell_time {0};

    graphics::ColoredSprites m_sprites;
    graphics::Sprites m_emoji_sprites;
    graphics::Shape m_boxes;
    terminal::Caret m_caret;  // visual indicator of cursor position
    graphics::Shape m_frame;  // for visual bell
};


} // namespace xci::widgets


#endif // XCI_WIDGETS_TEXTTERMINAL_H
