// Page.h created on 2018-03-18, part of XCI toolkit
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

#ifndef XCI_TEXT_LAYOUT_PAGE_H
#define XCI_TEXT_LAYOUT_PAGE_H

#include <xci/text/Font.h>
#include <xci/text/Style.h>
#include <xci/graphics/View.h>
#include <xci/util/geometry.h>

#include <vector>

namespace xci {
namespace text {
namespace layout {

class Span;
class Layout;

using ElementIndex = size_t;

enum class Alignment {
    Left,
    Right,
    Center,
    Justify,
};


class Page {
public:
    explicit Page(Layout& layout);

    // Target view which will be queried for sizes
    // If not set (nullptr), some generic, probably wrong sizes will be used.
    void set_target(const graphics::View* target) { m_target = target; }
    util::Vec2f target_pixel_ratio() const;

    // Reset all state
    void clear();

    // Acknowledge that we've started processing next element
    void advance_element();

    // Element index of next element to be typeset
    ElementIndex element_index() const { return m_element_index; }

    // ------------------------------------------------------------------------

    // Text style
    void set_font(Font* font) { m_style.set_font(font); }
    void set_font_size(float size) { m_style.set_size(size); }
    void set_color(const graphics::Color &color) { m_style.set_color(color); }
    void set_style(const Style& style) { m_style = style; }
    const Style& style() const { return m_style; }

    // Set page width. This drives the line breaking.
    // Default: 0 (same as INF - no line breaking)
    void set_width(float width) { m_width = width; }
    float width() const { return m_width; }

    void set_alignment(Alignment alignment) { m_alignment = alignment; }
    Alignment get_alignment() const { return m_alignment; }

    void add_tab_stop(float x);
    void reset_tab_stops() { m_tab_stops.clear(); }

    // ------------------------------------------------------------------------

    // Pen is a position in page where elements are printed
    void set_pen(util::Vec2f pen) { m_pen = pen; }
    const util::Vec2f& pen() const { return m_pen; }

    // Advance pen. The relative coords should be positive, don't move back.
    void advance_pen(util::Vec2f advance) { m_pen += advance; }

    // Finish current line, apply alignment and move to next one.
    // Does nothing if current line is empty.
    void finish_line();

    // Add vertical space.
    void advance_line(float lines = 1.0f);

    // Add a space after last word. Does nothing if current line is empty.
    void add_space(float spaces = 1.0f);

    // Put horizontal tab onto line. It takes all space up to next tabstop.
    void add_tab();

    // ------------------------------------------------------------------------

    void set_element_bounds(const util::Rect_f& word_bounds);

private:
    float space_width();

private:
    Layout& m_layout;
    const graphics::View* m_target = nullptr;

    // running state
    ElementIndex m_element_index = 0;
    util::Vec2f m_pen;  // pen position
    Style m_style;  // text style

    // page attributes
    float m_width = 0.0f;  // page width
    Alignment m_alignment = Alignment::Left;  // horizontal alignment
    std::vector<float> m_tab_stops;

    // page content
    std::vector<Span> m_lines;
};


}}} // namespace xci::text::layout

#endif // XCI_TEXT_LAYOUT_PAGE_H
