// Theme.cpp created on 2018-04-10, part of XCI toolkit
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

#include "Theme.h"

namespace xci {
namespace widgets {


Theme& Theme::default_theme()
{
    static Theme theme;
    return theme;
}


bool Theme::load_font(const char* file_path, int face_index)
{
    if (!m_font_face.load_from_file(file_path, face_index))
        return false;
    m_font.add_face(m_font_face);
    return true;
}


bool Theme::load_icon_font(const char* file_path, int face_index)
{
    if (!m_icon_font_face.load_from_file(file_path, face_index))
        return false;
    m_icon_font.add_face(m_icon_font_face);
    return true;
}


void Theme::set_icon_codepoint(IconId icon, CodePoint codepoint)
{
    m_icon_map[(size_t) icon] = codepoint;
}


CodePoint Theme::icon_codepoint(IconId icon_id)
{
    if (icon_id == IconId::None)
        return ' ';
    return m_icon_map[(size_t) icon_id];
}


#define TRY(stmt)  do { auto res = stmt; if (!res) return false; } while(0)

bool Theme::load_default_theme()
{
    Theme& theme = Theme::default_theme();

    // Base font
    TRY(theme.load_font(XCI_SHARE_DIR "/fonts/ShareTechMono/ShareTechMono-Regular.ttf", 0));

    // Material Icons
    TRY(theme.load_icon_font(XCI_SHARE_DIR "/fonts/MaterialIcons/MaterialIcons-Regular.woff", 0));
    theme.set_icon_codepoint(IconId::CheckBoxUnchecked, L'\ue835');
    theme.set_icon_codepoint(IconId::CheckBoxChecked, L'\ue834');
    theme.set_icon_codepoint(IconId::RadioButtonUnchecked, L'\ue836');
    theme.set_icon_codepoint(IconId::RadioButtonChecked, L'\ue837');

    return true;
}


}} // namespace xci::widgets
