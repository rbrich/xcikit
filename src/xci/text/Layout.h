// Layout.h created on 2018-03-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_LAYOUT_H
#define XCI_TEXT_LAYOUT_H

#include "layout/Page.h"
#include "layout/Element.h"
#include "Font.h"
#include "Style.h"
#include <xci/graphics/View.h>
#include <xci/graphics/Color.h>
#include <xci/graphics/shape/Rectangle.h>
#include <xci/math/Vec2.h>
#include <xci/core/container/ChunkedStack.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace xci::text {
namespace layout {

using graphics::VariUnits;
using graphics::VariCoords;
using graphics::Color;
using graphics::View;

// Layout allows to record a stream of elements (text and control)
// and then apply this stream of elements to generate precise positions
// bounding boxes according to current View, and draw them into the View.
class Layout
{
public:
    // Clear all contents.
    void clear();

    // These are not affected by `clear`
    Layout& set_default_page_width(VariUnits width);
    Layout& set_default_font(Font* font);
    Layout& set_default_font_size(VariUnits size, bool allow_scale = true);
    Layout& set_default_font_style(FontStyle font_style);
    Layout& set_default_font_weight(uint16_t weight);
    Layout& set_default_color(Color color);
    Layout& set_default_outline_radius(VariUnits radius);
    Layout& set_default_outline_color(Color color);
    Layout& set_default_tab_stops(std::vector<VariUnits> stops);
    Layout& set_default_alignment(Alignment alignment);

    const Style& default_style() const { return m_default_style; }

    // ------------------------------------------------------------------------
    // Control elements
    //
    // The following methods add control elements into the stream.
    // The new state will affect text elements added after this.

    // Set page width. This drives the line breaking.
    // Default: 0 (same as INF - no line breaking)
    void set_page_width(VariUnits width);

    // Set text alignment
    void set_alignment(Alignment alignment);

    // Set line spacing (multiple of default line height)
    void set_line_spacing(float multiplier);

    // Horizontal tab stops. Following Tab elements will add horizontal space
    // up to next tab stop.
    void add_tab_stop(VariUnits x);
    void reset_tab_stops();

    // Horizontal/vertical offset (in multiplies of font size)
    // This can be used to create subscript/superscript.
    void set_offset(VariSize offset);
    void reset_offset() { set_offset({0_fb, 0_fb}); }

    // Move to absolute position. Implies finish_line().
    void move_to(VariCoords coords);

    // Set font and text style.
    // Also affects spacing (which depends on font metrics).
    void set_font(Font* font);
    void set_font_size(VariUnits size);
    void set_font_style(FontStyle font_style);
    void set_bold(bool bold = true);
    void set_italic(bool italic = true);
    void set_color(Color color);
    void reset_color();

    // Word should be an actual word. Punctuation can be attached to it
    // or pushed separately as another "word". No whitespace should be contained
    // in the word, unless it is meant to behave as hard, unbreakable space.
    void add_word(std::string_view word);

    // Add a space after last word. Does nothing if current line is empty.
    void add_space();

    // Put horizontal tab onto line. It takes all space up to next tabstop.
    void add_tab();

    // Add a new line
    void new_line(float lines = 1.0f) { finish_line(); advance_line(lines); }

    // Finish current line, apply alignment and move to line beginning.
    // Does not add vertical space! This is only "carriage return".
    // Does nothing if current line is empty.
    void finish_line();

    // Add vertical space ("line feed").
    void advance_line(float lines = 1.0f);

    // ------------------------------------------------------------------------
    // Spans allow naming a part of the text and change its attributes later

    /// Begin a span
    /// The name should be unique - this is not checked.
    void begin_span(const std::string& name);

    /// End a span
    /// Ends last open span of the name (in case the name wasn't unique)
    /// \returns false if no span of the name was found
    bool end_span(const std::string& name);

    /// Get a span previously created by begin_span, end_span
    /// \returns first span of the name, NULL if the span does not exist
    Span* get_span(const std::string& name);

    /// Get view of all span names, in order of creation
    const std::vector<std::string>& span_names() const { return m_span_names; }

    // ------------------------------------------------------------------------
    // Typeset and draw

    // Typeset the element stream for the target, i.e. compute element
    // positions and sizes.
    // Should be called on every change of framebuffer size
    // and after addition of new elements.
    // Use also to realign/reflow after changing width or alignment.
    void typeset(const View& target);

    // Recreate graphics objects. Must be called at least once before draw.
    void update(const View& target);

    // Draw whole layout to target
    void draw(View& view, VariCoords pos) const;

    // ------------------------------------------------------------------------
    // Metrics

    FramebufferRect bbox() const;

private:
    Page m_page;
    std::vector<std::unique_ptr<Element>> m_elements;
    std::vector<std::string> m_span_names;

    Style m_default_style;
    VariUnits m_default_width = 0_px;
    Alignment m_default_alignment = Alignment::Left;
    std::vector<VariUnits> m_default_tab_stops;

    mutable core::ChunkedStack<graphics::Rectangle> m_debug_rects;
};


} // namespace layout

// Export Layout into xci::text namespace
using layout::Layout;

} // namespace xci::text

#endif // XCI_TEXT_LAYOUT_H
