// Layout.h created on 2018-03-10, part of XCI toolkit
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

#ifndef XCI_TEXT_LAYOUT_H
#define XCI_TEXT_LAYOUT_H

#include "layout/Page.h"
#include "layout/Element.h"
#include "Font.h"
#include "Style.h"
#include <xci/graphics/View.h>
#include <xci/graphics/Color.h>
#include <xci/util/geometry.h>

#include <string>
#include <utility>
#include <vector>

namespace xci {
namespace text {
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
    void set_default_page_width(float width);
    void set_default_font(Font* font);
    void set_default_font_size(float size);
    void set_default_color(const graphics::Color &color);

    // ------------------------------------------------------------------------
    // Control elements
    //
    // The following methods add control elements into the stream.
    // The new state will affect text elements added after this.

    // Set page width. This drives the line breaking.
    // Default: 0 (same as INF - no line breaking)
    void set_page_width(float width);

    // Set text alignment
    void set_alignment(Alignment alignment);

    // Horizontal tab stops. Following Tab elements will add horizontal space
    // up to next tab stop.
    void add_tab_stop(float x);
    void reset_tab_stops();

    // Horizontal/vertical offset (in multiplies of font size)
    // This can be used to create subscript/superscript.
    void set_offset(const util::Vec2f offset);
    void reset_offset() { set_offset({0.f, 0.f}); }

    // Set font and text style.
    // Also affects spacing (which depends on font metrics).
    void set_font(Font* font);
    void set_font_size(float size);
    void set_color(const graphics::Color &color);
    void reset_color();

    // ------------------------------------------------------------------------
    // Text elements

    // Word should be actual word. Punctuation can be attached to it
    // or pushed separately as another "word". No whitespace should be contained
    // in the word, unless it is meant to behave as hard, unbreakable space.
    void add_word(const std::string& string);

    // Add a space after last word. Does nothing if current line is empty.
    void add_space();

    // Put horizontal tab onto line. It takes all space up to next tabstop.
    void add_tab();

    // Finish current line, apply alignment and move to next one.
    // Does nothing if current line is empty.
    void finish_line();

    // ------------------------------------------------------------------------
    // Spans allow to name part of the text and change its attributes later

    // Begin and end the span.
    // Returns false on error:
    // - Trying to begin a span of same name twice.
    // - Trying to end not-started span.
    void begin_span(const std::string& name);
    void end_span(const std::string& name);

    // Returns NULL if the span does not exist.
    Span* get_span(const std::string& name);

    // ------------------------------------------------------------------------
    // Typeset and draw

    // Typeset the element stream for the target, ie. compute element
    // positions and sizes.
    // Should be called on every change of framebuffer size
    // and after addition of new elements.
    void typeset(const graphics::View& target);

    // Draw whole layout to target
    void draw(graphics::View& target, const util::Vec2f& pos) const;

    // ------------------------------------------------------------------------
    // Metrics

    util::Rect_f bbox() const;

private:
    Page m_page;
    std::vector<std::unique_ptr<Element>> m_elements;

    Style m_default_style;
    float m_default_width = 0;
};


} // namespace layout

// Export Layout into xci::text namespace
using layout::Layout;

}} // namespace xci::text

#endif // XCI_TEXT_LAYOUT_H
