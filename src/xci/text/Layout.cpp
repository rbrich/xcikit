// Layout.cpp created on 2018-03-10, part of XCI toolkit
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

#include "Layout.h"

#include <cassert>
#include <xci/graphics/Sprites.h>

namespace xci {
namespace text {


LayoutWord::Metrics LayoutWord::get_metrics() const
{
    Metrics metrics;

    // Word bounds are local, move by origin
    metrics.bounds.x = m_origin.x;
    metrics.bounds.y = m_origin.y;

    m_font->set_size(m_size);
    for (CodePoint code_point : m_string) {
        auto glyph = m_font->get_glyph(code_point);

        // Expand text bounds by glyph bounds
        util::Rect_f m;
        m.x = m_origin.x + metrics.advance.x + glyph->base_x();
        m.y = m_origin.y - glyph->base_y();
        m.w = glyph->width();
        m.h = glyph->height();  // ft_to_float(gm.height)
        metrics.bounds.extend(m);

        metrics.advance.x += glyph->advance();
    }
    return metrics;
}

void LayoutWord::draw(graphics::View& target, const util::Vec2f& pos) const
{
    if (!m_font)
        return;
    m_font->set_size(m_size);

    graphics::Sprites sprites(m_font->get_texture());

    Vec2f pen;
    for (CodePoint code_point : m_string) {
        auto glyph = m_font->get_glyph(code_point);
        if (glyph == nullptr)
            continue;

        sprites.add_sprite({pen.x + glyph->base_x(),
                            pen.y - glyph->base_y()},
                           glyph->tex_coords(),
                           m_color);

#if 0
        sf::RectangleShape bbox;
        sf::FloatRect m;
        m.left = ft_to_float(gm.horiBearingX) + pen;
        m.top = -ft_to_float(gm.horiBearingY);
        m.width = ft_to_float(gm.width);
        m.height = ft_to_float(gm.height);
        bbox.setPosition(m.left, m.top);
        bbox.setSize({m.width, m.height});
        bbox.setFillColor(sf::Color::Transparent);
        bbox.setOutlineColor(sf::Color::Blue);
        bbox.setOutlineThickness(1);
        target.draw(bbox, states);
#endif

        pen.x += glyph->advance();
    }

    sprites.draw(target, pos + m_origin);
}


// ----------------------------------------------------------------------------


LayoutSpan::LayoutSpan(Layout* m_layout, size_t begin)
        : m_layout(m_layout), m_begin(begin)
{
    assert(m_layout != nullptr);
}

void LayoutSpan::set_color(const graphics::Color &color)
{
    for (auto i = m_begin; i != m_end; i++) {
        m_layout->m_words[i].set_color(color);
    }
}

util::Rect_f LayoutSpan::get_bounds() const
{
    return m_bounds;
}

void LayoutSpan::add_padding(float radius)
{
    m_bounds.enlarge(radius);
}


// ----------------------------------------------------------------------------


Layout::Layout()
{
    m_lines.emplace_back(this, 0);
}


void Layout::clear()
{
    m_width = 0;
    pen = {0, 0};
    align = Align::Left;
    m_words.clear();
    m_lines = {LayoutSpan(this, 0)};
    m_spans.clear();
}


void Layout::add_tab_stop(float x)
{
    m_tab_stops.push_back(x);
    std::sort(m_tab_stops.begin(), m_tab_stops.end());
}


void Layout::add_word(const std::string &string)
{
    // Prepare the word
    LayoutWord word(string, m_font, m_size, m_color, pen);
    auto metrics = word.get_metrics();

    // Check line end
    if (m_width > 0.0 && pen.x + metrics.advance.x > m_width) {
        finish_line();
        word.set_origin(pen);
        metrics = word.get_metrics();
    }

    auto& line = m_lines.back();
    if (line.is_empty()) {
        line.m_bounds = {pen.x, pen.y, 0, 0};
    }
    line.m_bounds.extend(metrics.bounds);

    m_words.push_back(std::move(word));
    size_t word_index = m_words.size();
    line.set_end(word_index);
    pen += metrics.advance;

    // Add the word to currently open spans
    for (auto &span_pair : m_spans) {
        auto &span = span_pair.second;
        if (span.is_open()) {
            if (span.begin_index() == word_index) {
                span.m_bounds = metrics.bounds;
            } else {
                span.m_bounds.extend(metrics.bounds);
            }
        }
    }
}


void Layout::add_space()
{
    if (m_lines.back().is_empty())
        return;
    pen.x += space_width();
}


void Layout::add_tab()
{
    // apply tab stops
    auto tab_stop = m_tab_stops.begin();
    float x = 0;
    while (x < pen.x && tab_stop != m_tab_stops.end()) {
        x = *tab_stop++;
    }
    // apply generic tabs
    if (x < pen.x) {
        float tab_size = 8 * space_width();
        while (x < pen.x)
            x += tab_size;
    }
    // move to new position
    pen.x = x;
}


void Layout::finish_line()
{
    if (m_lines.back().is_empty())
        return;
    m_lines.emplace_back(this, m_words.size());
    pen.x = 0;
    pen.y += line_height();
}


void Layout::advance_lines(float lines)
{
    pen.y += lines * line_height();
}


bool Layout::begin_span(const std::string& name)
{
    auto result = m_spans.emplace(name, LayoutSpan(this, m_words.size()));
    return result.second;  // false if already existed
}


bool Layout::end_span(const std::string& name)
{
    auto iter = m_spans.find(name);
    if (iter == m_spans.end())
        return false;  // does not exist
    iter->second.set_end(m_words.size());
    return true;
}


LayoutSpan* Layout::get_span(const std::string& name)
{
    auto iter = m_spans.find(name);
    if (iter == m_spans.end())
        return nullptr;  // does not exist
    return &iter->second;
}


void Layout::draw(graphics::View& target, const util::Vec2f& pos) const
{
    for (auto& word : m_words) {
        word.draw(target, pos);

#if 0
        sf::CircleShape basepoint(2);
        basepoint.setFillColor(sf::Color::Magenta);
        target.draw(basepoint, translation);

        auto metrics = word.text.get_metrics();
        sf::RectangleShape bbox;
        bbox.setPosition(metrics.bounds.left, metrics.bounds.top);
        bbox.setSize({metrics.bounds.width, metrics.bounds.height});
        bbox.setFillColor(sf::Color::Transparent);
        bbox.setOutlineColor(sf::Color::Blue);
        bbox.setOutlineThickness(1);
        target.draw(bbox, translation);
//#endif
    }

//#ifndef NDEBUG
    if (d_show_bounds) {
        for (auto& span_item : m_spans) {
            auto& span = span_item.second;
            auto bounds = span.get_bounds();
            sf::RectangleShape bbox;
            bbox.setPosition(bounds.left, bounds.top);
            bbox.setSize({bounds.width, bounds.height});
            bbox.setFillColor(sf::Color::Transparent);
            bbox.setOutlineColor(sf::Color::Blue);
            bbox.setOutlineThickness(1);
            target.draw(bbox);
        }
#endif
    }
}

float Layout::space_width() const
{
    LayoutWord word(" ", m_font, m_size);
    auto metrics = word.get_metrics();
    return metrics.advance.x;
}

float Layout::line_height() const
{
    m_font->set_size(m_size);
    return m_font->line_height();
}


}} // namespace xci::text
