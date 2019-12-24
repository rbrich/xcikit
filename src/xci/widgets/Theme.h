// Theme.h created on 2018-04-10, part of XCI toolkit
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

#ifndef XCI_WIDGETS_THEME_H
#define XCI_WIDGETS_THEME_H

#include <xci/text/Font.h>
#include <xci/graphics/Color.h>
#include <xci/core/Vfs.h>
#include <array>

namespace xci::graphics { class Renderer; }

namespace xci::widgets {

enum class IconId {
    None,
    CheckBoxUnchecked,
    CheckBoxChecked,
    RadioButtonUnchecked,
    RadioButtonChecked,

    _NumItems_,
};

constexpr size_t IconMapSize = static_cast<size_t>(IconId::_NumItems_);
using IconMap = std::array<text::CodePoint, IconMapSize>;


enum class ColorId {
    Default,
    Hover,
    Focus,

    _NumItems_,
};

constexpr size_t ColorMapSize = static_cast<size_t>(ColorId::_NumItems_);
using ColorMap = std::array<graphics::Color, ColorMapSize>;


class Theme {
public:
    explicit Theme(graphics::Renderer& renderer);

    graphics::Renderer& renderer() const { return m_renderer; }

    // default theme
    bool load_default();

    // base font
    bool load_font_face(const core::Vfs& vfs, const char* file_path, int face_index);
    text::Font& font() { return m_font; }

    // icons
    bool load_icon_font_face(const core::Vfs& vfs, const char* file_path, int face_index);
    text::Font& icon_font() { return m_icon_font; }
    void set_icon_codepoint(IconId icon_id, text::CodePoint codepoint);
    text::CodePoint icon_codepoint(IconId icon_id);

    // colors
    void set_color(ColorId color_id, graphics::Color color);
    graphics::Color color(ColorId color_id);

private:
    graphics::Renderer& m_renderer;
    // base font
    text::Font m_font;
    // icons
    text::Font m_icon_font;
    IconMap m_icon_map {};
    // colors
    ColorMap m_color_map;
};


} // namespace xci::widgets

#endif // XCI_WIDGETS_THEME_H
