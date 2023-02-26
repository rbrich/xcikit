// Layout.cpp created on 2018-03-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Layout.h"

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


Layout& Layout::set_default_page_width(VariUnits width)
{
    m_default_width = width;
    m_page.clear();
    return *this;
}


Layout& Layout::set_default_font(Font* font)
{
    m_default_style.set_font(font);
    m_page.clear();
    return *this;
}


Layout& Layout::set_default_font_size(VariUnits size, bool allow_scale)
{
    m_default_style.set_size(size);
    m_default_style.set_allow_scale(allow_scale);
    m_page.clear();
    return *this;
}


Layout& Layout::set_default_font_style(FontStyle font_style)
{
    m_default_style.set_font_style(font_style);
    m_page.clear();
    return *this;
}


Layout& Layout::set_default_font_weight(uint16_t weight)
{
    m_default_style.set_font_weight(weight);
    m_page.clear();
    return *this;
}


Layout& Layout::set_default_color(graphics::Color color)
{
    m_default_style.set_color(color);
    m_page.clear();
    return *this;
}


Layout& Layout::set_default_outline_radius(VariUnits radius)
{
    m_default_style.set_outline_radius(radius);
    m_page.clear();
    return *this;
}


Layout& Layout::set_default_outline_color(graphics::Color color)
{
    m_default_style.set_outline_color(color);
    m_page.clear();
    return *this;
}


Layout& Layout::set_default_tab_stops(std::vector<VariUnits> stops)
{
    m_default_tab_stops = std::move(stops);
    m_page.clear();
    return *this;
}


Layout& Layout::set_default_alignment(Alignment alignment)
{
    m_default_alignment = alignment;
    m_page.clear();
    return *this;
}


void Layout::typeset(const View& target)
{
    m_page.clear();
    m_page.set_target(&target);
    m_page.set_width(target.to_fb(m_default_width));
    m_page.set_style(m_default_style);
    m_page.set_alignment(m_default_alignment);

    m_page.reset_tab_stops();
    for (auto stop : m_default_tab_stops)
        m_page.add_tab_stop(target.to_fb((stop)));

    for (auto& elem : m_elements) {
        elem->apply(m_page);
    }
    m_page.finish_line();
}

void Layout::update(const View& target)
{
    m_page.foreach_word([&](Word& word) {
        word.update(target);
    });

    // setup debug rectangles

    auto& renderer = target.window()->renderer();
    const auto fb_1px = target.px_to_fb(1_px);
    m_debug_rects.clear();

    // Debug: page bbox
    if (target.has_debug_flag(View::Debug::PageBBox)) {
        m_debug_rects.emplace_back(renderer);
        m_debug_rects.back().add_rectangle(bbox(), fb_1px);
        m_debug_rects.back().update(Color(150, 150, 0, 128),
                                    Color(200, 200, 50));
    }

    // Debug: span bboxes
    if (target.has_debug_flag(View::Debug::SpanBBox)) {
        m_debug_rects.emplace_back(renderer);
        m_page.foreach_span([&](const Span& span) {
            for (const auto& part : span.parts()) {
                m_debug_rects.back().add_rectangle(part.bbox(), fb_1px);
            }
        });
        m_debug_rects.back().update(Color(100, 0, 150, 128),
                                    Color(200, 50, 250));
    }

    // Debug: line bboxes
    if (target.has_debug_flag(View::Debug::LineBBox)) {
        m_debug_rects.emplace_back(renderer);
        m_page.foreach_line([&](const Line& line) {
            m_debug_rects.back().add_rectangle(line.bbox(), fb_1px);
        });
        m_debug_rects.back().update(Color(0, 50, 150, 128),
                                    Color(50, 50, 250));
    }

    // Debug: line baselines
    if (target.has_debug_flag(View::Debug::LineBaseLine)) {
        m_debug_rects.emplace_back(renderer);
        m_page.foreach_line([&](const Line& line) {
            auto rect = line.bbox();
            rect.y += line.baseline();
            rect.h = fb_1px;
            m_debug_rects.back().add_rectangle(rect);
        });
        m_debug_rects.back().update(Color(255, 50, 150));
    }
}


void Layout::draw(View& view, VariCoords pos) const
{
    for (auto& rects : m_debug_rects) {
        rects.draw(view, pos);
    }

    m_page.foreach_word([&](const Word& word) {
        word.draw(view, view.to_fb(pos));
    });
}


void Layout::set_page_width(VariUnits width)
{
    m_elements.push_back(std::make_unique<SetPageWidth>(width));
}


void Layout::set_alignment(Alignment alignment)
{
    m_elements.push_back(std::make_unique<SetAlignment>(alignment));
}


void Layout::set_line_spacing(float multiplier)
{
    m_elements.push_back(std::make_unique<SetLineSpacing>(multiplier));
}


void Layout::add_tab_stop(VariUnits x)
{
    m_elements.push_back(std::make_unique<AddTabStop>(x));
}


void Layout::reset_tab_stops()
{
    m_elements.push_back(std::make_unique<ResetTabStops>());
}


void Layout::set_offset(VariSize offset)
{
    m_elements.push_back(std::make_unique<SetOffset>(offset));
}


void Layout::move_to(VariCoords coords)
{
    m_elements.push_back(std::make_unique<MoveTo>(coords));
}


void Layout::set_font(Font* font)
{
    m_elements.push_back(std::make_unique<SetFont>(font));
}


void Layout::set_font_size(VariUnits size)
{
    m_elements.push_back(std::make_unique<SetFontSize>(size));
}


void Layout::set_font_style(FontStyle font_style)
{
    m_elements.push_back(std::make_unique<SetFontStyle>(font_style));
}


void Layout::set_bold(bool bold)
{
    m_elements.push_back(std::make_unique<SetBold>(bold));
}


void Layout::set_italic(bool italic)
{
    m_elements.push_back(std::make_unique<SetItalic>(italic));
}


void Layout::set_color(graphics::Color color)
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


void Layout::advance_line(float lines)
{
    m_elements.push_back(std::make_unique<AdvanceLine>(lines));
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


FramebufferRect Layout::bbox() const
{
    FramebufferRect bbox;
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
