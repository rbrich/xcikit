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

#include <xci/graphics/Rectangles.h>

#include <cassert>
#include <xci/graphics/View.h>

namespace xci {
namespace text {
namespace layout {

using xci::graphics::Color;
using xci::graphics::View;

/*
void Span::set_color(const graphics::Color& color)
{
    for (auto i = m_begin; i != m_end; i++) {
        // FIXME
        //m_layout->m_elements[i].set_color(color);
    }
}
*/

util::Rect_f Span::bounds() const
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


void Layout::set_default_page_width(float width)
{
    m_default_width = width;
    m_page.clear();
}


void Layout::set_default_font(Font* font)
{
    m_default_style.set_font(font);
    m_page.clear();
}


void Layout::set_default_font_size(float size)
{
    m_default_style.set_size(size);
    m_page.clear();
}


void Layout::set_default_color(const graphics::Color& color)
{
    m_default_style.set_color(color);
    m_page.clear();
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
    m_page.set_width(m_default_width);
    m_page.set_style(m_default_style);

    for (auto elem = m_elements.begin() + m_page.element_index();
         elem != m_elements.end(); elem++)
    {
        (*elem)->apply(m_page);
        m_page.advance_element();
    }
}


void Layout::draw(View& target, const util::Vec2f& pos) const
{
    auto pxr = target.pixel_ratio();

    // Debug: page bbox
    if (target.has_debug_flag(View::Debug::PageBBox)) {
        util::Rect_f bounds;
        graphics::Rectangles bbox(Color(150, 150, 0, 128), Color(200, 200, 50));
        for (auto& line : m_page.lines()) {
            if (bounds.empty())
                bounds = line.bounds();
            else
                bounds.extend(line.bounds());
        }
        bbox.add_rectangle(bounds, 1 * pxr.x);
        bbox.draw(target, pos);
    }

    // Debug: line bboxes
    if (target.has_debug_flag(View::Debug::LineBBox)) {
        for (auto& line : m_page.lines()) {
            graphics::Rectangles bbox(Color(0, 50, 150, 128), Color(50, 50, 250));
            bbox.add_rectangle(line.bounds(), 1 * pxr.x);
            bbox.draw(target, pos);
        }
    }

    for (auto& elem : m_elements) {
        elem->draw(target, pos);
    }

#if 0
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


void Layout::add_space()
{
    m_elements.push_back(std::make_unique<Space>());
}


void Layout::add_tab()
{
    m_elements.push_back(std::make_unique<Tab>());
}


void Layout::finish_line()
{
    m_elements.push_back(std::make_unique<FinishLine>());
}


}}} // namespace xci::text::layout
