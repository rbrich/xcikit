// Text.h created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_TEXT_H
#define XCI_TEXT_TEXT_H

#include "Layout.h"
#include <xci/core/geometry/Vec2.h>
#include <xci/graphics/View.h>

#include <string>

namespace xci::graphics { struct Color; }
namespace xci::text { class Font; }

namespace xci::text {

using graphics::VariUnits;
using graphics::VariCoords;

// Text rendering - convenient combination of Layout and Markup
class Text {
public:
    enum class Format {
        None,   // interpret nothing (\n is char 0x10 in the font)
        Plain,  // interpret just C escapes (\n, \t)
        Markup, // interpret control sequences etc. (see Markup.h)
    };

    Text() = default;
    Text(Font& font, const std::string &string, Format format = Format::Plain);

    void set_string(const std::string& string, Format format = Format::Plain);
    void set_fixed_string(const std::string& string) { set_string(string, Format::None); }
    void set_markup_string(const std::string& string) { set_string(string, Format::Markup); }

    void set_width(VariUnits width);
    void set_font(Font& font);
    void set_font_size(VariUnits size, bool allow_scale = true);
    void set_font_style(FontStyle font_style);
    void set_font_weight(uint16_t weight);
    void set_color(graphics::Color color);
    void set_outline_radius(VariUnits radius);
    void set_outline_color(graphics::Color color);
    void set_tab_stops(std::vector<VariUnits> stops);
    void set_alignment(Alignment alignment);

    Layout& layout() { return m_layout; }

    void resize(graphics::View& view);
    void update(graphics::View& view);
    void draw(graphics::View& view, VariCoords pos);

private:
    Layout m_layout;
    bool m_need_typeset = false;
};


} // namespace xci::text

#endif // XCI_TEXT_TEXT_H
