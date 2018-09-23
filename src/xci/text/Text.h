// Text.h created on 2018-03-02, part of XCI toolkit
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

#ifndef XCI_TEXT_TEXT_H
#define XCI_TEXT_TEXT_H

#include "Layout.h"
#include <xci/core/geometry.h>

#include <string>

namespace xci::graphics { class View; }
namespace xci::graphics { struct Color; }
namespace xci::text { class Font; }

namespace xci::text {


// Text rendering - convenient combination of Layout and Markup
class Text {
public:
    Text() = default;
    Text(const std::string &string, Font& font);

    void set_string(const std::string& string);
    void set_fixed_string(const std::string& string);

    void set_width(float width) { m_layout.set_default_page_width(width); }
    void set_font(Font& font) { m_layout.set_default_font(&font); }
    void set_size(float size) { m_layout.set_default_font_size(size); }
    void set_color(const graphics::Color& color) { m_layout.set_default_color(color); }

    Layout& layout() { return m_layout; }

    void resize(graphics::View& view);
    void draw(graphics::View& view, const core::Vec2f& pos);
    void resize_draw(graphics::View& view, const core::Vec2f& pos);

private:
    Layout m_layout;
};


} // namespace xci::text

#endif // XCI_TEXT_TEXT_H
