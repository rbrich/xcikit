// Layout.h created on 2018-03-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_LAYOUT_H
#define XCI_TEXT_LAYOUT_H

#include "layout/Page.h"
#include "layout/Element.h"
#include "Font.h"
#include "Style.h"
#include <xci/graphics/View.h>
#include <xci/graphics/Color.h>
#include <xci/graphics/Shape.h>
#include <xci/core/geometry.h>
#include <xci/core/container/ChunkedStack.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace xci::text {
namespace layout {


// Layout allows to record a stream of elements (text and control)
// and then apply this stream of elements to generate precise positions
// bounding boxes according to current View, and draw them into the View.
class Layout
{
public:
    // Clear all contents.
    void clear();

    // These are not affected by `clear`
    void set_default_page_width(ViewportUnits width);
    void set_default_font(Font* font);
    void set_default_font_size(ViewportUnits size, bool allow_scale = true);
    void set_default_font_style(FontStyle font_style);
    void set_default_color(graphics::Color color);
    void set_default_tab_stops(std::vector<ViewportUnits> stops);
    void set_default_alignment(Alignment alignment);

    const Style& default_style() const { return m_default_style; }

    // ------------------------------------------------------------------------
    // Control elements
    //
    // The following methods add control elements into the stream.
    // The new state will affect text elements added after this.

    // Set page width. This drives the line breaking.
    // Default: 0 (same as INF - no line breaking)
    void set_page_width(ViewportUnits width);

    // Set text alignment
    void set_alignment(Alignment alignment);

    // Horizontal tab stops. Following Tab elements will add horizontal space
    // up to next tab stop.
    void add_tab_stop(ViewportUnits x);
    void reset_tab_stops();

    // Horizontal/vertical offset (in multiplies of font size)
    // This can be used to create subscript/superscript.
    void set_offset(const ViewportSize& offset);
    void reset_offset() { set_offset({0.f, 0.f}); }

    // Set font and text style.
    // Also affects spacing (which depends on font metrics).
    void set_font(Font* font);
    void set_font_size(ViewportUnits size);
    void set_font_style(FontStyle font_style);
    void set_bold(bool bold = true);
    void set_italic(bool italic = true);
    void set_color(graphics::Color color);
    void reset_color();

    // ------------------------------------------------------------------------
    // Text elements

    // Word should be actual word. Punctuation can be attached to it
    // or pushed separately as another "word". No whitespace should be contained
    // in the word, unless it is meant to behave as hard, unbreakable space.
    void add_word(std::string_view word);

    // Add a space after last word. Does nothing if current line is empty.
    void add_space();

    // Put horizontal tab onto line. It takes all space up to next tabstop.
    void add_tab();

    // Finish current line, apply alignment and move to next one.
    // Does nothing if current line is empty.
    void finish_line();

    // Add vertical space. Implies `finish_line`.
    void advance_line(float lines = 1.0f);

    // ------------------------------------------------------------------------
    // Spans allow naming a part of the text and change its attributes later

    /// Begin the span
    /// Logs error when trying to begin a span of the same name twice
    void begin_span(const std::string& name);

    /// End the span
    /// Logs error when trying to end a span which doesn't exist or was already ended
    void end_span(const std::string& name);

    /// Get a span previously created by begin_span, end_span
    /// \returns NULL if the span does not exist
    Span* get_span(const std::string& name);

    // ------------------------------------------------------------------------
    // Typeset and draw

    // Typeset the element stream for the target, i.e. compute element
    // positions and sizes.
    // Should be called on every change of framebuffer size
    // and after addition of new elements.
    // Use also to realign/reflow after changing width or alignment.
    void typeset(const graphics::View& target);

    // Recreate graphics objects. Must be called at least once before draw.
    void update(const graphics::View& target);

    // Draw whole layout to target
    void draw(graphics::View& target, const ViewportCoords& pos) const;

    // ------------------------------------------------------------------------
    // Metrics

    ViewportRect bbox() const;

private:
    Page m_page;
    std::vector<std::unique_ptr<Element>> m_elements;

    Style m_default_style;
    ViewportUnits m_default_width = 0;
    Alignment m_default_alignment = Alignment::Left;
    std::vector<ViewportUnits> m_default_tab_stops;

    mutable core::ChunkedStack<graphics::Shape> m_debug_shapes;
};


} // namespace layout

// Export Layout into xci::text namespace
using layout::Layout;

} // namespace xci::text

#endif // XCI_TEXT_LAYOUT_H
