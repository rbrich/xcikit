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
namespace layout {


void Word::apply(Page& page)
{
    // Measure word (metrics are affected by string, font, size)
    util::Vec2f advance;
    util::Rect_f bounds;
    auto* font = page.style().font();
    font->set_size(unsigned(page.style().size() * 600));
    for (CodePoint code_point : m_string) {
        auto glyph = font->get_glyph(code_point);

        // Expand text bounds by glyph bounds
        util::Rect_f m;
        m.x = advance.x + glyph->base_x() / 300.0f;
        m.y = 0.0f - glyph->base_y() / 300.0f;
        m.w = glyph->width() / 300.0f;
        m.h = glyph->height() / 300.0f;  // ft_to_float(gm.height)
        bounds.extend(m);

        advance.x += glyph->advance() / 300.0f;
    }

    // Check line end
    if (page.width() > 0.0 && page.pen().x + advance.x > page.width()) {
        page.finish_line();
        page.advance_line();
    }

    // Set position according to pen
    m_pos = page.pen();
    bounds.x += m_pos.x;
    bounds.y += m_pos.y;

    page.set_element_bounds(bounds);
    page.advance_pen(advance);

    m_style = page.style();
}


void Word::draw(graphics::View& target, const util::Vec2f& pos) const
{
    auto* font = m_style.font();
    if (!font)
        return;
    font->set_size(unsigned(m_style.size() * 600));

    graphics::Sprites sprites(font->get_texture());

    Vec2f pen;
    for (CodePoint code_point : m_string) {
        auto glyph = font->get_glyph(code_point);
        if (glyph == nullptr)
            continue;

        sprites.add_sprite({pen.x + glyph->base_x() / 300.0f,
                            pen.y - glyph->base_y() / 300.0f,
                            glyph->width() / 300.0f,
                            glyph->height() / 300.0f},
                           glyph->tex_coords(),
                           m_style.color());

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

        pen.x += glyph->advance() / 300.0f;
    }

    sprites.draw(target, pos + m_pos);
}


// ----------------------------------------------------------------------------

/*
void Span::set_color(const graphics::Color& color)
{
    for (auto i = m_begin; i != m_end; i++) {
        // FIXME
        //m_layout->m_elements[i].set_color(color);
    }
}
*/

util::Rect_f Span::get_bounds() const
{
    return m_bounds;
}

void Span::add_padding(float radius)
{
    m_bounds.enlarge(radius);
}


// ----------------------------------------------------------------------------


void Layout::clear()
{
    m_page.clear();
    m_elements.clear();
    m_spans.clear();
}


bool Layout::begin_span(const std::string& name)
{
    auto result = m_spans.emplace(name, Span(m_elements.size()));
    return result.second;  // false if already existed
}


bool Layout::end_span(const std::string& name)
{
    auto iter = m_spans.find(name);
    if (iter == m_spans.end())
        return false;  // does not exist
    iter->second.set_end(m_elements.size());
    return true;
}


Span* Layout::get_span(const std::string& name)
{
    auto iter = m_spans.find(name);
    if (iter == m_spans.end())
        return nullptr;  // does not exist
    return &iter->second;
}


void Layout::typeset(const graphics::View& target)
{
    // TODO: detect if target size changed and don't clear page if it didn't
    m_page.clear();
    m_page.set_target(&target);

    for (auto elem = m_elements.begin() + m_page.element_index();
         elem != m_elements.end(); elem++)
    {
        (*elem)->apply(m_page);
        m_page.advance_element();
    }
}


void Layout::draw(graphics::View& target, const util::Vec2f& pos) const
{
    for (auto& elem : m_elements) {
        elem->draw(target, pos);

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


void Layout::set_page_width(float width)
{
    m_elements.push_back(std::make_unique<SetPageWidth>(width));
}

void Layout::set_alignment(Alignment alignment)
{
    m_elements.push_back(std::make_unique<SetAlignment>(alignment));
}

void Layout::add_tab_stop(float x)
{
    m_elements.push_back(std::make_unique<AddTabStop>(x));
}

void Layout::reset_tab_stops()
{
    m_elements.push_back(std::make_unique<ResetTabStops>());
}

void Layout::set_font(Font* font)
{
    m_elements.push_back(std::make_unique<SetFont>(font));
}

void Layout::set_font_size(float size)
{
    m_elements.push_back(std::make_unique<SetFontSize>(size));
}

void Layout::set_color(const graphics::Color& color)
{
    m_elements.push_back(std::make_unique<SetColor>(color));
}

void Layout::add_word(const std::string& string)
{
    m_elements.push_back(std::make_unique<Word>(string));
}


Page::Page(Layout& layout)
    : m_layout(layout)
{
    m_lines.emplace_back(0);
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
    font->set_size(unsigned(m_style.size() * 600));
    float line_height = font->line_height() / 300.0f;
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

float Page::space_width()
{
    auto* font = m_style.font();
    font->set_size(unsigned(m_style.size() * 600));
    auto glyph = font->get_glyph(' ');
    return glyph->advance() / 300.0f;
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


}}} // namespace xci::text::layout
