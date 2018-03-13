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
#include "../graphics/View.h"
#include "../graphics/Color.h"
#include "../util/geometry.h"

#include <string>
#include <vector>

namespace xci {
namespace text {


class Layout;


// Single word, consisting of letters (glyphs), font and style.
class LayoutWord {
public:
    LayoutWord(const std::string& string,
               Font* font,
               unsigned size = 12,
               const graphics::Color& color = graphics::Color::White(),
               const util::Vec2f& origin = {0, 0})
        : m_string(string), m_font(font), m_size(size), m_color(color),
          m_origin(origin) {}

    void set_color(const graphics::Color& color) { m_color = color; }
    const graphics::Color& color() const { return m_color; }

    void set_origin(const util::Vec2f& origin) { m_origin = origin; }

    // Measure text (metrics are affected by string, font, size)
    struct Metrics {
        util::Vec2f advance;
        util::Rect_f bounds;
    };
    Metrics get_metrics() const;

    void draw(graphics::View& target, const util::Vec2f& pos) const;

private:
    std::string m_string;
    Font* m_font;
    unsigned m_size;
    graphics::Color m_color;
    util::Vec2f m_origin;  // relative to page origin (top-left corner)
};


// Group of words, allowing mass editing and providing a bounding box
struct LayoutSpan {
    using WordIndex = size_t;
    static constexpr size_t invalid_index = ~0u;

    explicit LayoutSpan(Layout* m_layout, WordIndex begin);

    void set_end(WordIndex end) { m_end = end; }

    WordIndex begin_index() const { return m_begin; }
    WordIndex end_index() const { return m_end; }

    bool is_empty() const { return m_begin == m_end; }
    bool is_open() const { return m_begin != invalid_index && m_end == invalid_index; }

    // Restyle all words in span
    void set_color(const graphics::Color &color);

    // Retrieve adjusted (global) bounding box of the span.
    util::Rect_f get_bounds() const;

    // add padding to bounds
    void add_padding(float radius);

    friend class Layout;

private:
    Layout *m_layout;
    WordIndex m_begin;
    WordIndex m_end = invalid_index;  // points after last work in STL fashion
    util::Rect_f m_bounds;
};


// Push text into Layout, read back the positions of lines, words, glyphs
// Drawing is done by other object, this handles just the placement.
// Parsing text is also done by other object, here, any sequence of whitespace
// characters is translated to a single space.
class Layout {
public:
    Layout();

    // Clear all state, start over.
    void clear();

    // Set page width. This drives the line breaking.
    // Default: 0 (same as INF - no line breaking)
    void set_width(float width) { m_width = width; reflow(); }
    float get_width() const { return m_width; }

    // Set alignment
    enum class Align {
        Left,
        Right,
        Center,
        Justify,
    };
    void set_align(Align alignment) { align = alignment; realign(); }
    Align get_align() const { return align; }

    // Advance pen. The relative coords should be positive, don't move back.
    void move_pen(float rx, float ry) { pen.x += rx; pen.y += ry; }
    const util::Vec2f &get_pen() const { return pen; }

    void add_tab_stop(float x);

    // Set font and style to be recorded with every following word.
    // Also affects spacing (which depends on font size).
    void set_font(Font& font) { m_font = &font; }

    void set_size(unsigned size) { m_size = size; }
    unsigned size() const { return m_size; }

    void set_color(const graphics::Color &color) { m_color = color; }
    const graphics::Color& color() const { return m_color; }

    // Put a word on pen position.
    void add_word(const std::string &string);

    // Add a space after last word. Does nothing if current line is empty.
    void add_space();

    // Put horizontal tab onto line. It takes all space up to next tabstop.
    void add_tab();

    // Finish current line, apply alignment and move to new one.
    // Does nothing if current line is empty.
    void finish_line();

    // Add vertical space. Before this, call `finish_line`.
    void advance_lines(float lines = 1.0f);

    // ------------------------------------------------------------------------
    // Spans allow to name part of the text and change its attributes later

    // Begin and end the span.
    // Returns false on error:
    // - Trying to begin a span of same name twice.
    // - Trying to end not-started span.
    bool begin_span(const std::string &name);
    bool end_span(const std::string &name);

    // Returns NULL if the span does not exist.
    LayoutSpan* get_span(const std::string &name);

    friend struct LayoutSpan;

    // ------------------------------------------------------------------------
    // Draw

    void draw(graphics::View& target, const util::Vec2f& pos) const;

    // ------------------------------------------------------------------------
    // Debug

#ifndef NDEBUG
    bool d_show_bounds = false;
#endif

private:
    // pen position
    util::Vec2f pen;

    // text style
    Font* m_font = nullptr;
    unsigned m_size = 12;
    graphics::Color m_color = graphics::Color::White();

    // page parameters
    float m_width = 0.f;  // page width
    Align align = Align::Left;  // horizontal alignment
    std::vector<float> m_tab_stops;

    // page content
    std::vector<LayoutWord> m_words;
    std::vector<LayoutSpan> m_lines;
    std::map<std::string, LayoutSpan> m_spans;

    float space_width() const;
    float line_height() const;

    // Apply new alignment on already laid out text
    void realign() {}

    // Apply new page witdth on already laid out text
    void reflow() {}
};


}} // namespace xci::text

#endif // XCI_TEXT_LAYOUT_H
