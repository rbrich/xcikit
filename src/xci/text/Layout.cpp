// Layout.cpp created on 2018-03-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

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

void Layout::update(const graphics::View& target)
{
    m_page.foreach_word([&](Word& word) {
        word.update(target);
    });

    // setup debug rectangles

    auto& renderer = target.window()->renderer();
    const auto sc_1px = target.size_to_viewport(1_sc);
    m_debug_shapes.clear();

    // Debug: page bbox
    if (target.has_debug_flag(View::Debug::PageBBox)) {
        m_debug_shapes.emplace_back(renderer,
                Color(150, 150, 0, 128),
                Color(200, 200, 50));
        m_debug_shapes.back().add_rectangle(bbox(), sc_1px);
        m_debug_shapes.back().update();
    }

    // Debug: span bboxes
    if (target.has_debug_flag(View::Debug::SpanBBox)) {
        m_debug_shapes.emplace_back(renderer,
                Color(100, 0, 150, 128),
                Color(200, 50, 250));
        m_page.foreach_span([&](const Span& span) {
            for (const auto& part : span.parts()) {
                m_debug_shapes.back().add_rectangle(part.bbox(), sc_1px);
            }
        });
        m_debug_shapes.back().update();
    }

    // Debug: line bboxes
    if (target.has_debug_flag(View::Debug::LineBBox)) {
        m_debug_shapes.emplace_back(renderer,
                Color(0, 50, 150, 128),
                Color(50, 50, 250));
        m_page.foreach_line([&](const Line& line) {
            m_debug_shapes.back().add_rectangle(line.bbox(), sc_1px);
        });
        m_debug_shapes.back().update();
    }

    // Debug: line baselines
    if (target.has_debug_flag(View::Debug::LineBaseLine)) {
        m_debug_shapes.emplace_back(renderer, Color(255, 50, 150));
        m_page.foreach_line([&](const Line& line) {
            auto rect = line.bbox();
            rect.y += line.baseline();
            rect.h = sc_1px;
            m_debug_shapes.back().add_rectangle(rect);
        });
        m_debug_shapes.back().update();
    }
}


void Layout::draw(View& target, const ViewportCoords& pos) const
{
    for (auto& shape : m_debug_shapes) {
        shape.draw(target, pos);
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


void Layout::add_word(std::string_view word)
{
    m_elements.push_back(std::make_unique<AddWord>(std::string(word)));
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
