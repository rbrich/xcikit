// Style.cpp created on 2018-03-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Style.h"


namespace xci::text {

using namespace graphics;


void Style::clear()
{
    m_font = nullptr;
    m_size = 0.05f;
    m_scale = 1.0f;
    m_color = graphics::Color::White();
    m_font_style = FontStyle::Regular;
}


void Style::apply_view(const View& view)
{
    // must select style (i.e. font face) before changing size of the font face
    m_font->set_style(m_font_style);
    auto font_size = view.size_to_framebuffer(m_size);
    m_font->set_size(unsigned(std::ceilf(font_size.value)));
    m_scale = m_allow_scale ? font_size.value / m_font->height() : 1.0f;
}


} // namespace xci::text
