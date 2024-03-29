// Page.h created on 2018-03-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_LAYOUT_PAGE_H
#define XCI_TEXT_LAYOUT_PAGE_H

#include <xci/text/Style.h>
#include <xci/graphics/Sprites.h>
#include <xci/graphics/shape/Rectangle.h>
#include <xci/math/Vec2.h>
#include <xci/core/container/ChunkedStack.h>

#include <string>
#include <vector>
#include <optional>

namespace xci::graphics { class View; }
namespace xci::text { class Font; }

namespace xci::text::layout {

using graphics::FramebufferPixels;
using graphics::FramebufferCoords;
using graphics::FramebufferSize;
using graphics::FramebufferRect;

class Layout;
class Page;


class Word {
public:
    Word(Page& page, const std::string& utf8);

    FramebufferRect bbox() const { return m_bbox.moved(m_pos); }
    FramebufferPixels baseline() const { return m_baseline; }
    Style& style() { return m_style; }

    // Reposition the word on x-axis
    void move_x(FramebufferPixels offset);

    void update(const graphics::View& target);
    void draw(graphics::View& target, FramebufferCoords pos) const;

private:
    std::vector<FontFace::GlyphPlacement> m_shaped;
    Style m_style;
    FramebufferCoords m_pos;  // relative to page (top-left corner)
    FramebufferRect m_bbox;
    FramebufferPixels m_baseline = 0;  // relative to bbox top

    mutable std::optional<graphics::Sprites> m_sprites;
    mutable std::optional<graphics::Sprites> m_outline_sprites;
    mutable core::ChunkedStack<graphics::Rectangle> m_debug_rects;
};


class Line {
public:
    void add_word(Word& word) { m_words.push_back(&word); m_bbox_valid = false; }
    std::vector<Word*>& words() { return m_words; }

    bool is_empty() const { return m_words.empty(); }

    // Retrieve bounding box of the whole line, relative to page
    const FramebufferRect& bbox() const;
    FramebufferPixels baseline() const;

    // Align content of the line
    void align(Alignment alignment, FramebufferPixels width);

    // Padding to be added to each side of the bounding box
    void set_padding(FramebufferPixels padding) { m_padding = padding; m_bbox_valid = false; }

private:
    std::vector<Word*> m_words;
    FramebufferPixels m_padding = 0;
    mutable FramebufferRect m_bbox;
    mutable bool m_bbox_valid = false;
};


// Group of words, spanning one or more lines.
// Allows mass editing of the line parts and words in span.
class Span {
public:
    Span() { add_part(); }

    void add_word(Word& word);

    void add_part() { m_parts.emplace_back(); }
    const Line& part(int idx) const { return m_parts[idx]; }
    const std::vector<Line>& parts() const { return m_parts; }

    void close() { m_open = false; }
    bool is_open() const { return m_open; }

    // Restyle all words in span.
    // The callback will be run on each word in the span,
    // with reference to the word's current style to be adjusted.
    void adjust_style(const std::function<void(Style& word_style)>& fn_adjust);

    // Convenience shortcuts for `adjust_style`:
    void adjust_color(graphics::Color c) { adjust_style([c](Style& w){ w.set_color(c); }); }

    bool contains(FramebufferCoords point) const;

private:
    std::vector<Line> m_parts;
    bool m_open = true;
};


class Page {
public:
    explicit Page();

    // Target view which will be queried for sizes
    // If not set (nullptr), some generic, probably wrong sizes will be used.
    void set_target(const graphics::View* target) { m_target = target; }
    const graphics::View& target() const;

    // Reset all state
    void clear();

    // ------------------------------------------------------------------------

    // Text style
    void set_font(Font* font) { m_style.set_font(font); }
    void set_font_size(VariUnits size) { m_style.set_size(size); }
    void set_font_style(FontStyle font_style) { m_style.set_font_style(font_style); }
    void set_color(graphics::Color color) { m_style.set_color(color); }
    void set_style(const Style& style) { m_style = style; }
    const Style& style() const { return m_style; }

    // Set page width. This drives the line breaking.
    // Default: 0 (same as INF - no line breaking)
    void set_width(FramebufferPixels width) { m_width = width; }
    FramebufferPixels width() const { return m_width; }

    void set_alignment(Alignment alignment) { m_alignment = alignment; }
    Alignment get_alignment() const { return m_alignment; }

    void set_line_spacing(float multiplier) { m_line_spacing = multiplier; }

    // Tab stops are relative to origin
    void add_tab_stop(FramebufferPixels x);
    void reset_tab_stops() { m_tab_stops.clear(); }

    // ------------------------------------------------------------------------

    // Origin is a position in page where current run started (on line-break, pen returns to origin)
    // Also moves pen to new origin.
    void set_origin(FramebufferCoords origin) { m_origin = origin; m_pen = origin; }
    FramebufferCoords origin() const { return m_origin; }

    // Pen is a position in page where elements are printed
    void set_pen(FramebufferCoords pen) { m_pen = pen; m_pen_offset = {}; }
    FramebufferCoords pen() const { return m_pen + m_pen_offset; }

    // Advance pen. The relative coords should be positive, don't move back.
    void advance_pen(FramebufferSize advance) { m_pen += advance; }

    // Offset pen position. Can be used for subscript/superscript etc.
    void set_pen_offset(FramebufferSize pen_offset) { m_pen_offset = pen_offset; }
    FramebufferSize pen_offset() const { return m_pen_offset; }

    // Finish current line, apply alignment and move to line beginning.
    // Does not add vertical space! This is only "carriage return".
    // Does nothing if current line is empty.
    void finish_line();

    // Add vertical space ("line feed").
    void advance_line(float lines = 1.0f);

    // Add a space after last word. Does nothing if current line is empty.
    void add_space(float spaces = 1.0f);

    // Put horizontal tab onto line. It takes all space up to next tabstop.
    void add_tab();

    // Add word bbox to line bbox
    void add_word(const std::string& string);

    // ------------------------------------------------------------------------
    // Spans allow to mark part of the text and change its attributes later

    using SpanIndex = unsigned int;

    /// Begin new span
    /// \returns index of the span
    SpanIndex begin_span();

    /// End a span previously started with `begin_span`
    /// \param index  Span index as returned from begin_span.
    /// \returns false is the index is invalid or the span is already closed
    bool end_span(SpanIndex index);

    /// Get a span previously created by begin_span, end_span
    /// \returns Span with the index or nullptr if the index is invalid
    Span* get_span(SpanIndex index);

    // ------------------------------------------------------------------------

    void foreach_word(const std::function<void(Word& word)>& cb);
    void foreach_word(const std::function<void(const Word& word)>& cb) const;
    void foreach_line(const std::function<void(const Line& line)>& cb) const;
    void foreach_span(const std::function<void(const Span& span)>& cb) const;

private:
    FramebufferPixels space_width();

private:
    const graphics::View* m_target = nullptr;

    // running state
    FramebufferCoords m_origin;  // origin where pen started (used for "carriage return")
    FramebufferCoords m_pen;  // pen position
    FramebufferSize m_pen_offset;
    Style m_style;  // text style
    float m_line_spacing = 1.0f;

    // page attributes
    FramebufferPixels m_width = 0;  // page width
    Alignment m_alignment = Alignment::Left;  // horizontal alignment
    std::vector<FramebufferPixels> m_tab_stops;

    // page content
    core::ChunkedStack<Word> m_words;
    std::vector<Line> m_lines;
    std::vector<Span> m_spans;
};


} // namespace xci::text::layout

#endif // XCI_TEXT_LAYOUT_PAGE_H
