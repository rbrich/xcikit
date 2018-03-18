// Page.cpp created on 2018-03-18, part of XCI toolkit
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

#include "Page.h"
#include "../Layout.h"

namespace xci {
namespace text {
namespace layout {


Page::Page(Layout& layout)
        : m_layout(layout)
{
    m_lines.emplace_back(0);
}


util::Vec2f Page::target_pixel_ratio() const
{
    if (!m_target)
        return {300, 300};

    return m_target->pixel_ratio();
}


void Page::clear()
{
    m_element_index = 0;
    m_pen = {0, 0};
    m_style.clear();
    m_width = 0;
    m_alignment = Alignment::Left;
    m_tab_stops.clear();
    m_lines.clear();
    m_lines.emplace_back(0);
}


void Page::advance_element()
{
    m_element_index++;
    m_lines.back().set_end(m_element_index);
}


void Page::add_tab_stop(float x)
{
    m_tab_stops.push_back(x);
    std::sort(m_tab_stops.begin(), m_tab_stops.end());
}


void Page::finish_line()
{
    if (m_lines.back().is_empty())
        return;
    m_lines.emplace_back(m_element_index);
    m_pen.x = 0;
    advance_line();
}


void Page::advance_line(float lines)
{
    auto* font = m_style.font();
    auto pxr = target_pixel_ratio();
    font->set_size(unsigned(m_style.size() * pxr.y));
    float line_height = font->line_height() / pxr.y;
    m_pen.y += lines * line_height;
}


void Page::add_space(float spaces)
{
    if (m_lines.back().is_empty())
        return;
    m_pen.x += space_width();
}


void Page::add_tab()
{
    // apply tab stops
    auto tab_stop = m_tab_stops.begin();
    float x = 0;
    while (x < m_pen.x && tab_stop != m_tab_stops.end()) {
        x = *tab_stop++;
    }
    // apply generic tabs
    if (x < m_pen.x) {
        float tab_size = 8 * space_width();
        while (x < m_pen.x)
            x += tab_size;
    }
    // move to new position
    m_pen.x = x;
}


void Page::set_element_bounds(const util::Rect_f& word_bounds)
{
    // Extend line bounds
    auto& line = m_lines.back();
    if (line.is_empty()) {
        line.m_bounds = word_bounds;
    } else {
        line.m_bounds.extend(word_bounds);
    }

    // Extend bounds of relevant spans
    for (auto& span_pair : m_layout.m_spans) {
        auto& span = span_pair.second;
        if (span.begin_index() == m_element_index) {
            span.m_bounds = word_bounds;
        } else
        if (span.begin_index() > m_element_index && span.end_index() <= m_element_index) {
            span.m_bounds.extend(word_bounds);
        }
    }
}


float Page::space_width()
{
    auto* font = m_style.font();
    auto pxr = target_pixel_ratio();
    font->set_size(unsigned(m_style.size() * pxr.y));
    auto glyph = font->get_glyph(' ');
    return glyph->advance() / pxr.x;
}


}}} // namespace xci::text::layout
