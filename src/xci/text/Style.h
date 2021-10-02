// Style.h created on 2018-03-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_STYLE_H
#define XCI_TEXT_STYLE_H

#include "Font.h"
#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>

namespace xci::text {

using graphics::ViewportUnits;
using namespace graphics::unit_literals;


class Style {
public:
    void clear();

    void set_font(Font* font) { m_font = font; }
    Font* font() const { return m_font; }

    // Request font size
    void set_size(ViewportUnits size) { m_size = size; }
    ViewportUnits size() const { return m_size; }

    void set_color(const graphics::Color &color) { m_color = color; }
    const graphics::Color& color() const { return m_color; }

    // Update the font to the selected size
    void apply_view(const graphics::View& view);

    // Computed ratio: requested size / actual font height
    // Multiply font metrics by scale to get actual screen metrics
    float scale() const { return m_scale; }

private:
    Font* m_font = nullptr;
    ViewportUnits m_size = 0.05_vp;  // requested font height
    float m_scale = 1.0f;
    graphics::Color m_color = graphics::Color::White();
};


} // namespace xci::text

#endif // XCI_TEXT_STYLE_H
