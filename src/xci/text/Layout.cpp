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

namespace xci {
namespace text {


void Layout::clear()
{
    m_width = 0;
    pen = {0, 0};
    align = Align::Left;
    m_words.clear();
    m_lines = {LayoutLine()};
    m_spans.clear();
}


void Layout::add_tab_stop(float x)
{
    m_tab_stops.push_back(x);
    std::sort(m_tab_stops.begin(), m_tab_stops.end());
}


void Layout::add_word(const std::string &word)
{
    // Prepare the word
    Text text;
    text.set_font(m_font);
    text.set_size(m_size);
    text.set_color(m_color);
    text.set_string(word);
    auto metrics = text.get_metrics();

    // Check line end
    if (m_width > 0.0 && pen.x + metrics.advance.x > m_width) {
        finish_line();
    }

    // Word bounds are local, move by pen (ie. add word origin)
    metrics.bounds.x += pen.x;
    metrics.bounds.y += pen.y;

    auto& line = m_lines.back();
    if (line.word_indices.empty()) {
        line.bounds = {pen.x, pen.y, 0, 0};
    }
    line.bounds.extend(metrics.bounds);

    size_t word_index = m_words.size();
    line.word_indices.push_back(word_index);
    m_words.emplace_back(text, pen);
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
    if (m_lines.back().word_indices.empty())
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
    if (m_lines.back().word_indices.empty())
        return;
    m_lines.emplace_back();
    pen.x = 0;
    pen.y += line_height();
}


void Layout::advance_lines(float lines)
{
    pen.y += lines * line_height();
}


void Layout::draw(graphics::View& target, const util::Vec2f& pos) const
{
    for (auto& word : m_words) {
        word.text.draw(target, pos + word.origin);
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
    Text text;
    text.set_font(m_font);
    text.set_size(m_size);
    text.set_string(" ");
    auto metrics = text.get_metrics();
    return metrics.advance.x;
}

float Layout::line_height() const
{
    m_font->set_size(m_size);
    return m_font->line_height();
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


LayoutSpan::LayoutSpan(Layout* m_layout, size_t begin)
        : m_layout(m_layout), m_begin(begin)
{
    assert(m_layout != nullptr);
}

void LayoutSpan::set_color(const graphics::Color &color)
{
    for (auto i = m_begin; i != m_end; i++) {
        m_layout->m_words[i].text.set_color(color);
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


}} // namespace xci::text
