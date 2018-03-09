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

#include <xci/text/Font.h>
#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>
#include <xci/util/geometry.h>

#include <string>

namespace xci {
namespace text {


// Text rendering
class Text {
public:
    using Color = graphics::Color;

    // Set string
    void set_string(const std::string& string) { m_string = string; }

    // Font
    void set_font(Font* font) { m_font = font; }
    void set_font(Font& font) { m_font = &font; }

    // Size
    void set_size(unsigned size) { m_size = size; }
    unsigned size() const { return m_size; }

    // Color
    void set_color(const Color& color) { m_color = color; }
    const Color& color() const { return m_color; }

    // Measure text (metrics are affected by string, font, size)
    struct Metrics {
        util::Vec2f advance;
        util::Rect_f bounds;
    };
    Metrics get_metrics() const;

    void draw(graphics::View& target, const util::Vec2f& pos) const;

private:
    std::string m_string;
    Font* m_font;
    unsigned m_size = 12;
    Color m_color = Color::White();
};


}} // namespace xci::text

#endif // XCI_TEXT_TEXT_H
