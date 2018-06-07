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

#include <string>
#include <vector>
#include <list>

namespace xci {
namespace text {
namespace layout {

class Layout;
class Page;

using ElementIndex = size_t;

enum class Alignment {
    Left,
    Right,
    Center,
    Justify,
};


class Word {
public:
    Word(Page& page, const std::string& string);

    const util::Rect_f& bbox() const { return m_bbox; }
    Style& style() { return m_style; }

    void draw(graphics::View& target, const util::Vec2f& pos) const;

private:
    std::string m_string;
    Style m_style;
    util::Vec2f m_pos;  // relative to page origin (top-left corner)
    util::Rect_f m_bbox;
};


class Line {
public:
    void add_word(Word& word) { m_words.push_back(&word); m_bbox_valid = false; }
    std::vector<Word*>& words() { return m_words; }

    bool is_empty() const { return m_words.empty(); }

    // Retrieve bounding box of the span, relative to page
    const util::Rect_f& bbox() const;

    // Padding to be added to each side of the bounding box
    void set_padding(float padding) { m_padding = padding; m_bbox_valid = false; }

private:
    std::vector<Word*> m_words;
    float m_padding = 0;
    mutable util::Rect_f m_bbox;
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
    void adjust_style(std::function<void(Style& word_style)> fn_adjust);

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
    util::Vec2f target_framebuffer_ratio() const;

    // Reset all state
    void clear();

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
    void set_pen_offset(util::Vec2f pen_offset) { m_pen_offset = pen_offset; }
    util::Vec2f pen() const { return m_pen + m_pen_offset * m_style.size(); }

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

    // Add word bbox to line bbox
    void add_word(const std::string& string);

    // ------------------------------------------------------------------------
    // Spans allow to name part of the text and change its attributes later

    // Begin and end the span.
    // Returns false on error:
    // - Trying to begin a span of same name twice.
    // - Trying to end not-started span.
    bool begin_span(const std::string& name);
    bool end_span(const std::string& name);

    // Returns NULL if the span does not exist.
    Span* get_span(const std::string& name);

    // ------------------------------------------------------------------------

    void foreach_word(const std::function<void(const Word& word)>& cb) const;
    void foreach_line(const std::function<void(const Line& line)>& cb) const;
    void foreach_span(const std::function<void(const Span& span)>& cb) const;

private:
    float space_width();

private:
    const graphics::View* m_target = nullptr;

    // running state
    util::Vec2f m_pen;  // pen position
    util::Vec2f m_pen_offset;
    Style m_style;  // text style

    // page attributes
    float m_width = 0.f;  // page width
    Alignment m_alignment = Alignment::Left;  // horizontal alignment
    std::vector<float> m_tab_stops;

    // page content
    std::list<Word> m_words;
    std::vector<Line> m_lines;
    std::map<std::string, Span> m_spans;
};


}}} // namespace xci::text::layout

#endif // XCI_TEXT_LAYOUT_PAGE_H
