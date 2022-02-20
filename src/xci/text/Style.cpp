// Style.cpp created on 2018-03-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Style.h"
#include <cmath>


namespace xci::text {

using namespace graphics;


void Style::clear()
{
    m_font = nullptr;
    m_size = 0.05_vp;
    m_outline_radius = 0.0_vp;
    m_color = graphics::Color::White();
    m_outline_color = graphics::Color::Transparent();
    m_font_style = FontStyle::Regular;
    m_font_weight = 0;
    m_scale = 1.0f;
}


void Style::apply_view(const View& view)
{
    // must select style (i.e. font face) before changing size of the font face
    m_font->set_style(m_font_style);
    if (m_font_weight != 0)
        m_font->set_weight(m_font_weight);
    // convert font size
    auto font_size = view.size_to_framebuffer(m_size);
    m_font->set_size(unsigned(std::ceil(font_size.value)));
    m_scale = m_allow_scale ? font_size.value / m_font->height() : 1.0f;
    // two pass rendering - disable stroker for the first pass
    m_font->set_stroke(StrokeType::None, 0.0f);
}


void Style::apply_outline(const View& view)
{
    m_font->set_stroke(
            m_color.is_transparent() ? StrokeType::Outline : StrokeType::OutsideBorder,
            view.size_to_framebuffer(m_outline_radius).value);
}


} // namespace xci::text
