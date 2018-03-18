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

#include "Font.h"
#include "Style.h"
#include "../graphics/View.h"
#include "../graphics/Color.h"
#include "../util/geometry.h"

#include <string>
#include <utility>
#include <vector>

namespace xci {
namespace text {
namespace layout {


class Word;
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


class Element {
public:
    virtual void apply(Page& page) = 0;
    virtual void draw(graphics::View& target, const util::Vec2f& pos) const {}
};


// ------------------------------------------------------------------------
// Control elements - change page attributes

class SetPageWidth: public Element {
public:
    explicit SetPageWidth(float width) : m_width(width) {}
    void apply(Page& page) override {
        page.set_width(m_width);
    }

private:
    float m_width;
};


class SetAlignment: public Element {
public:
    explicit SetAlignment(Alignment alignment) : m_alignment(alignment) {}
    void apply(Page& page) override {
        page.set_alignment(m_alignment);
    }

private:
    Alignment m_alignment;
};


class AddTabStop: public Element {
public:
    explicit AddTabStop(float tab_stop) : m_tab_stop(tab_stop) {}
    void apply(Page& page) override {
        page.add_tab_stop(m_tab_stop);
    }

private:
    float m_tab_stop;
};


class ResetTabStops: public Element {
public:
    void apply(Page& page) override {
        page.reset_tab_stops();
    }
};


class SetFont: public Element {
public:
    explicit SetFont(Font* font) : m_font(font) {}
    void apply(Page& page) override {
        page.set_font(m_font);
    }

private:
    Font* m_font;
};


class SetFontSize: public Element {
public:
    explicit SetFontSize(float size) : m_size(size) {}
    void apply(Page& page) override {
        page.set_font_size(m_size);
    }

private:
    float m_size;
};


class SetColor: public Element {
public:
    explicit SetColor(graphics::Color color) : m_color(color) {}
    void apply(Page& page) override {
        page.set_color(m_color);
    }

private:
    graphics::Color m_color;
};


// ------------------------------------------------------------------------
// Text elements


// Single word, consisting of letters (glyphs), font and style.
class Word: public Element {
public:
    explicit Word(std::string string) : m_string(std::move(string)) {}

    void apply(Page& page) override;
    void draw(graphics::View& target, const util::Vec2f& pos) const;

private:
    std::string m_string;
    Style m_style;
    util::Vec2f m_pos;  // relative to page origin (top-left corner)
};


class Space: public Element {
public:
    void apply(Page& page) override { page.add_space(); }
};


class Tab: public Element {
public:
    void apply(Page& page) override { page.add_tab(); }
};


class FinishLine: public Element {
public:
    void apply(Page& page) override { page.finish_line(); }
};


// Group of words, allowing mass editing and providing a bounding box
class Span
{
public:
    static constexpr ElementIndex invalid_index = ~0u;

    explicit Span(ElementIndex begin) : m_begin(begin) {}

    void set_end(ElementIndex end) { m_end = end; }

    ElementIndex begin_index() const { return m_begin; }

    ElementIndex end_index() const { return m_end; }

    bool is_empty() const { return m_begin == m_end; }

    bool is_open() const
    {
        return m_begin != invalid_index && m_end == invalid_index;
    }

    // Restyle all words in span
    //void set_color(const graphics::Color& color);

    // Retrieve adjusted (global) bounding box of the span.
    util::Rect_f get_bounds() const;

    // add padding to bounds
    void add_padding(float radius);

private:
    friend class Page;

    ElementIndex m_begin;
    ElementIndex m_end = invalid_index;  // points after last work in STL fashion
    util::Rect_f m_bounds;
};


// Layout allows to record a stream of elements (text and control)
// and then apply this stream of elements to generate precise positions
// bounding boxes according to current View, and draw them into the View.
class Layout
{
public:
    Layout() : m_page(*this) {}

    // Clear all contents.
    void clear();

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

    // Set font and text style.
    // Also affects spacing (which depends on font metrics).
    void set_font(Font* font);
    void set_font_size(float size);
    void set_color(const graphics::Color &color);

    // ------------------------------------------------------------------------
    // Text elements

    // Word should be actual word. Punctuation can be attached to it
    // or pushed separately as another "word". No whitespace should be contained
    // in the word, unless it is meant to behave as hard, unbreakable space.
    void add_word(const std::string& string);

    // Add a space after last word. Does nothing if current line is empty.
    void add_space() { m_elements.push_back(std::make_unique<Space>()); }

    // Put horizontal tab onto line. It takes all space up to next tabstop.
    void add_tab() { m_elements.push_back(std::make_unique<Tab>()); }

    // Finish current line, apply alignment and move to new one.
    // Does nothing if current line is empty.
    void finish_line() { m_elements.push_back(std::make_unique<FinishLine>()); }

    // ------------------------------------------------------------------------
    // Spans allow to name part of the text and change its attributes later

    friend struct Span;

    // Begin and end the span.
    // Returns false on error:
    // - Trying to begin a span of same name twice.
    // - Trying to end not-started span.
    bool begin_span(const std::string& name);

    bool end_span(const std::string& name);

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
    // Debug

#ifndef NDEBUG
    bool d_show_bounds = false;
#endif

private:
    friend class Page;
    Page m_page;
    std::vector<std::unique_ptr<Element>> m_elements;
    std::map<std::string, Span> m_spans;
};


} // namespace layout

// Export Layout into xci::text namespace
using layout::Layout;

}} // namespace xci::text

#endif // XCI_TEXT_LAYOUT_H
