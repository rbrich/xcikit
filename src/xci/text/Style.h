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


enum class Alignment {
    Left,
    Right,
    Center,
    Justify,
};


class Style {
public:
    void clear();

    void set_font(Font* font) { m_font = font; }
    Font* font() const { return m_font; }

    // Request font size
    void set_size(ViewportUnits size) { m_size = size; }
    ViewportUnits size() const { return m_size; }

    // Set false to force using exact font size, without GPU scaling
    void set_allow_scale(bool allow_scale) { m_allow_scale = allow_scale; }
    bool allow_scale() const { return m_allow_scale; }

    // Set font style
    void set_font_style(FontStyle font_style) { m_font_style = font_style; }
    FontStyle font_style() const { return m_font_style; }

    // Font weight
    void set_font_weight(uint16_t weight) { m_font_weight = weight; }
    uint16_t font_weight() const { return m_font_weight; }

    // Fill color
    void set_color(graphics::Color color) { m_color = color; }
    graphics::Color color() const { return m_color; }

    // Outlined text
    // * outline color other than Transparent enables the outline
    // * set Transparent text color to get outlined text without inner filling
    // * set both colors to get a text with colored outside border
    void set_outline_radius(ViewportUnits radius) { m_outline_radius = radius; }
    ViewportUnits outline_radius() const { return m_outline_radius; }
    void set_outline_color(graphics::Color color) { m_outline_color = color; }
    graphics::Color outline_color() const { return m_outline_color; }

    // Update the font to the selected size
    void apply_view(const graphics::View& view);

    // Outline may require multi-pass rendering
    // * first, apply_view and render filled shape of the text into a buffer
    // * then, apply_outline and render the outline into a buffer
    // * draw outline buffer first, then blit the fill buffer over it
    void apply_outline(const graphics::View& view);

    // Computed ratio: requested size / actual font height
    // Multiply font metrics by scale to get actual screen metrics
    float scale() const { return m_scale; }

private:
    Font* m_font = nullptr;
    ViewportUnits m_size = 0.05_vp;  // requested font height
    ViewportUnits m_outline_radius = 0.0_vp;  // requested outline radius
    graphics::Color m_color = graphics::Color::White();
    graphics::Color m_outline_color = graphics::Color::Transparent();
    FontStyle m_font_style = FontStyle::Regular;
    uint16_t m_font_weight = 0;
    float m_scale = 1.0f;
    bool m_allow_scale = true;
};


} // namespace xci::text

#endif // XCI_TEXT_STYLE_H
