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

#include <xci/graphics/Shapes.h>
#include <xci/graphics/View.h>
#include <xci/util/compat.h>

#include <cassert>

namespace xci {
namespace text {
namespace layout {

using xci::graphics::Color;
using xci::graphics::View;


void Layout::clear()
{
    m_page.clear();
    m_elements.clear();
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


void Layout::draw(View& target, const util::Vec2f& pos) const
{
    auto pxr = target.screen_ratio();

    // Debug: page bbox
    if (target.has_debug_flag(View::Debug::PageBBox)) {
        graphics::Shapes bbox_rect(Color(150, 150, 0, 128), Color(200, 200, 50));
        bbox_rect.add_rectangle(bbox(), 1 * pxr.x);
        bbox_rect.draw(target, pos);
    }

    // Debug: span bboxes
    if (target.has_debug_flag(View::Debug::SpanBBox)) {
        graphics::Shapes bboxes(Color(100, 0, 150, 128), Color(200, 50, 250));
        for (auto* span : m_page.spans()) {
            for (auto& part : span->parts()) {
                bboxes.add_rectangle(part.bbox(), 1 * pxr.x);
            }
        }
        bboxes.draw(target, pos);
    }

    // Debug: line bboxes
    if (target.has_debug_flag(View::Debug::LineBBox)) {
        graphics::Shapes bboxes(Color(0, 50, 150, 128), Color(50, 50, 250));
        for (auto& line : m_page.lines()) {
            bboxes.add_rectangle(line.bbox(), 1 * pxr.x);
        }
        bboxes.draw(target, pos);
    }

    for (auto& word : m_page.words()) {
        word.draw(target, pos);
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


util::Rect_f Layout::bbox() const
{
    util::Rect_f bbox;
    for (auto& line : m_page.lines()) {
        if (bbox.empty())
            bbox = line.bbox();
        else
            bbox.extend(line.bbox());
    }
    return bbox;
}


}}} // namespace xci::text::layout
