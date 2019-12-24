// Text.h created on 2018-03-02, part of XCI toolkit
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

#ifndef XCI_TEXT_TEXT_H
#define XCI_TEXT_TEXT_H

#include "Layout.h"
#include <xci/core/geometry.h>
#include <xci/graphics/View.h>

#include <string>

namespace xci::graphics { struct Color; }
namespace xci::text { class Font; }

namespace xci::text {

using graphics::ViewportUnits;
using graphics::ViewportCoords;

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

    void set_width(ViewportUnits width);
    void set_font(Font& font);
    void set_font_size(ViewportUnits size);
    void set_color(const graphics::Color& color);

    Layout& layout() { return m_layout; }

    void resize(graphics::View& view);
    void update(graphics::View& view);
    void draw(graphics::View& view, const ViewportCoords& pos);

private:
    Layout m_layout;
    bool m_need_typeset = false;
};


} // namespace xci::text

#endif // XCI_TEXT_TEXT_H
