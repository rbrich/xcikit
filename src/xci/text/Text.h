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
#include <xci/text/Font.h>
#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>
#include <xci/util/geometry.h>

#include <string>

namespace xci {
namespace text {


// Text rendering - convenient combination of Layout and Markup
class Text {
public:
    void set_string(const std::string& string) { m_string = string; }
    void set_width(float width) { m_layout.set_width(width); }
    void set_font(Font& font) { m_layout.set_font(font); }
    void set_size(unsigned size) { m_layout.set_size(size); }
    void set_color(const graphics::Color& color) { m_layout.set_color(color); }

    void draw(graphics::View& target, const util::Vec2f& pos);

private:
    Layout m_layout;
    std::string m_string;
    bool m_parsed = false;
};


}} // namespace xci::text

#endif // XCI_TEXT_TEXT_H
