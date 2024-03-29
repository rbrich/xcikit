// Text.h created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_TEXT_H
#define XCI_TEXT_TEXT_H

#include "Layout.h"
#include <xci/math/Vec2.h>
#include <xci/graphics/View.h>

#include <string>

namespace xci::graphics { struct Color; }
namespace xci::text { class Font; }

namespace xci::text {

using graphics::VariUnits;
using graphics::VariCoords;

enum class TextFormat {
    None,   // interpret nothing (\n is char 0x10 in the font)
    Plain,  // interpret just C escapes (\n, \t)
    Markup, // interpret control sequences etc. (see Markup.h)
};


// Text rendering - convenient combination of Layout and Markup
class TextMixin {
public:
    void set_string(const std::string& string, TextFormat format = TextFormat::Plain);
    void set_fixed_string(const std::string& string) { set_string(string, TextFormat::None); }
    void set_markup_string(const std::string& string) { set_string(string, TextFormat::Markup); }

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

protected:
    void _resize(graphics::View& view);
    void _update(graphics::View& view);
    void _draw(graphics::View& view, VariCoords pos);

    Layout m_layout;
    bool m_need_typeset = false;
};


class Text: public TextMixin {
public:
    Text() = default;
    Text(Font& font, const std::string &string, TextFormat format = TextFormat::Plain);

    void resize(graphics::View& view) { view.finish_draw(); _resize(view); }
    void update(graphics::View& view) { view.finish_draw(); _update(view); }
    void draw(graphics::View& view, VariCoords pos) { _draw(view, pos); }
};


} // namespace xci::text

#endif // XCI_TEXT_TEXT_H
