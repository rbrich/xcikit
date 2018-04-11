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
#include <array>

namespace xci {
namespace widgets {

using namespace xci::text;

enum class IconId {
    CheckBoxUnchecked,
    CheckBoxChecked,
    RadioButtonUnchecked,
    RadioButtonChecked,

    None,
};

constexpr size_t IconMapSize = (size_t) IconId::None;
using IconMap = std::array<CodePoint, IconMapSize>;


class Theme {
public:
    static bool load_default_theme();
    static Theme& default_theme();

    bool load_font(const char* file_path, int face_index);
    Font& font() { return m_font; }

    bool load_icon_font(const char* file_path, int face_index);
    Font& icon_font() { return m_icon_font; }
    void set_icon_codepoint(IconId icon, CodePoint codepoint);
    CodePoint icon_codepoint(IconId icon_id);

private:
    // base font
    FontFace m_font_face;
    Font m_font;
    // icons
    FontFace m_icon_font_face;
    Font m_icon_font;
    IconMap m_icon_map;
};


}} // namespace xci::widgets

#endif // XCI_WIDGETS_THEME_H
