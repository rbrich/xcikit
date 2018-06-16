// TextInput.h created on 2018-06-02, part of XCI toolkit
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

#ifndef XCI_WIDGETS_TEXTINPUT_H
#define XCI_WIDGETS_TEXTINPUT_H

#include <xci/widgets/Widget.h>
#include <xci/text/Layout.h>
#include <xci/graphics/Shape.h>

namespace xci {
namespace widgets {


class TextInput: public Widget {
public:
    explicit TextInput(const std::string &string);

    void set_font_size(float size) { m_layout.set_default_font_size(size); }
    void set_width(float width) { m_width = width; }
    void set_padding(float padding) { m_padding = padding; }
    void set_outline_thickness(float thickness) { m_outline_thickness = thickness; }

    void set_decoration_color(const graphics::Color& fill, const graphics::Color& border);
    void set_text_color(const graphics::Color& color) { m_layout.set_default_color(color); }

    bool contains(const util::Vec2f& point) override;
    void update(View& view) override;
    void draw(View& view, State state) override;
    void handle(View& view, const KeyEvent& ev) override;
    void handle(View& view, const CharEvent& ev) override;
    void handle(View& view, const MouseBtnEvent& ev) override;

    util::Rect_f bbox() const;

private:
    std::string m_text;
    text::Layout m_layout;
    graphics::Shape m_bg_rect;
    graphics::Shape m_cursor_shape;
    size_t m_cursor = 0;
    float m_width = 0.2f;
    float m_padding = 0.02f;
    float m_content_pos = 0;
    float m_outline_thickness = 0.005f;
};


}} // namespace xci::widgets

#endif // XCI_WIDGETS_TEXTINPUT_H
