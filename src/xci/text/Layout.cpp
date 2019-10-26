// Layout.cpp created on 2018-03-10, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
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

#include <xci/graphics/Shape.h>
#include <xci/graphics/View.h>
#include <xci/graphics/Window.h>

#include <cassert>

namespace xci::text::layout {

using xci::graphics::Color;
using xci::graphics::View;
using namespace xci::graphics::unit_literals;


void Layout::clear()
{
    m_page.clear();
    m_elements.clear();
}


void Layout::set_default_page_width(ViewportUnits width)
{
    m_default_width = width;
    m_page.clear();
}


void Layout::set_default_font(Font* font)
{
    m_default_style.set_font(font);
    m_page.clear();
}


void Layout::set_default_font_size(ViewportUnits size)
{
    m_default_style.set_size(size);
    m_page.clear();
}


void Layout::set_default_color(const graphics::Color& color)
{
    m_default_style.set_color(color);
    m_page.clear();
}


void Layout::typeset(const graphics::View& target)
{
    m_page.clear();
    m_page.set_target(&target);
    m_page.set_width(m_default_width);
    m_page.set_style(m_default_style);

    for (auto& elem : m_elements) {
        elem->apply(m_page);
    }
}


void Layout::draw(View& target, const ViewportCoords& pos) const
{
    auto& renderer = target.window()->renderer();
    const auto sc_1px = target.size_to_viewport(1_sc);

    // Debug: page bbox
    if (target.has_debug_flag(View::Debug::PageBBox)) {
        graphics::Shape bbox_rect(renderer, Color(150, 150, 0, 128), Color(200, 200, 50));
        bbox_rect.add_rectangle(bbox(), sc_1px);
        bbox_rect.draw(target, pos);
    }

    // Debug: span bboxes
    if (target.has_debug_flag(View::Debug::SpanBBox)) {
        graphics::Shape bboxes(renderer, Color(100, 0, 150, 128), Color(200, 50, 250));
        m_page.foreach_span([&](const Span& span) {
            for (auto& part : span.parts()) {
                bboxes.add_rectangle(part.bbox(), sc_1px);
            }
        });
        bboxes.draw(target, pos);
    }

    // Debug: line bboxes
    if (target.has_debug_flag(View::Debug::LineBBox)) {
        graphics::Shape bboxes(renderer, Color(0, 50, 150, 128), Color(50, 50, 250));
        m_page.foreach_line([&](const Line& line) {
            bboxes.add_rectangle(line.bbox(), sc_1px);
        });
        bboxes.draw(target, pos);
    }

    // Debug line baselines
    if (target.has_debug_flag(View::Debug::LineBaseLine)) {
        graphics::Shape baselines(renderer, Color(255, 50, 150));
        m_page.foreach_line([&](const Line& line) {
            auto rect = line.bbox();
            rect.y += line.baseline();
            rect.h = sc_1px;
            baselines.add_rectangle(rect);
        });
        baselines.draw(target, pos);
    }

    m_page.foreach_word([&](const Word& word) {
        word.draw(target, pos);
    });
}


void Layout::set_page_width(ViewportUnits width)
{
    m_elements.push_back(std::make_unique<SetPageWidth>(width));
}


void Layout::set_alignment(Alignment alignment)
{
    m_elements.push_back(std::make_unique<SetAlignment>(alignment));
}


void Layout::add_tab_stop(ViewportUnits x)
{
    m_elements.push_back(std::make_unique<AddTabStop>(x));
}


void Layout::reset_tab_stops()
{
    m_elements.push_back(std::make_unique<ResetTabStops>());
}


void Layout::set_offset(const ViewportSize& offset)
{
    m_elements.push_back(std::make_unique<SetOffset>(offset));
}


void Layout::set_font(Font* font)
{
    m_elements.push_back(std::make_unique<SetFont>(font));
}


void Layout::set_font_size(ViewportUnits size)
{
    m_elements.push_back(std::make_unique<SetFontSize>(size));
}


void Layout::set_color(const graphics::Color& color)
{
    m_elements.push_back(std::make_unique<SetColor>(color));
}


void Layout::reset_color()
{
    m_elements.push_back(std::make_unique<SetColor>(m_default_style.color()));
}


void Layout::add_word(const std::string& string)
{
    m_elements.push_back(std::make_unique<AddWord>(string));
}


void Layout::add_space()
{
    m_elements.push_back(std::make_unique<AddSpace>());
}


void Layout::add_tab()
{
    m_elements.push_back(std::make_unique<AddTab>());
}


void Layout::finish_line()
{
    m_elements.push_back(std::make_unique<FinishLine>());
}


void Layout::begin_span(const std::string& name)
{
    m_elements.push_back(std::make_unique<BeginSpan>(name));
}


void Layout::end_span(const std::string& name)
{
    m_elements.push_back(std::make_unique<EndSpan>(name));
}


Span* Layout::get_span(const std::string& name)
{
    return m_page.get_span(name);
}


ViewportRect Layout::bbox() const
{
    ViewportRect bbox;
    bool first = true;
    m_page.foreach_line([&](const Line& line) {
        if (first) {
            bbox = line.bbox();
            first = false;
        } else {
            bbox.extend(line.bbox());
        }
    });
    return bbox;
}


} // namespace xci::text::layout
