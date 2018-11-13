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
#include <xci/core/Vfs.h>
#include <xci/core/file.h>
#include <xci/text/FontLibrary.h>

namespace xci {
namespace widgets {


#define TRY(stmt)  do { auto res = stmt; if (!res) return false; } while(0)

bool Theme::load_default_theme()
{
    Theme& theme = Theme::default_theme();

    // Base font
    TRY(theme.load_font_face("fonts/Hack/Hack-Regular.ttf", 0));
    TRY(theme.load_font_face("fonts/Hack/Hack-Bold.ttf", 0));
    TRY(theme.load_font_face("fonts/Hack/Hack-Italic.ttf", 0));
    TRY(theme.load_font_face("fonts/Hack/Hack-BoldItalic.ttf", 0));

    // Material Icons
    TRY(theme.load_icon_font_face("fonts/MaterialIcons/MaterialIcons-Regular.woff", 0));
    theme.set_icon_codepoint(IconId::None, L' ');
    theme.set_icon_codepoint(IconId::CheckBoxUnchecked, L'\ue835');
    theme.set_icon_codepoint(IconId::CheckBoxChecked, L'\ue834');
    theme.set_icon_codepoint(IconId::RadioButtonUnchecked, L'\ue836');
    theme.set_icon_codepoint(IconId::RadioButtonChecked, L'\ue837');

    // Colors
    theme.set_color(ColorId::Default, {180, 180, 180});
    theme.set_color(ColorId::Hover, graphics::Color::White());
    theme.set_color(ColorId::Focus, graphics::Color::Yellow());

    return true;
}


Theme& Theme::default_theme()
{
    static Theme theme;
    return theme;
}


static bool impl_load_font_face(text::Font& font, const char* file_path, int face_index)
{
    auto& vfs = core::Vfs::default_instance();
    auto font_face = text::FontLibrary::default_instance()->create_font_face();
    auto face_file = vfs.open(file_path);
    if (face_file.is_real_file()) {
        // it's a real file, use only the path, let FreeType read the data
        if (!font_face->load_from_file(face_file.path(), face_index))
            return false;
    } else {
        // not real file, we have to read all data into memory
        if (!font_face->load_from_memory(face_file.content(), face_index))
            return false;
    }
    font.add_face(std::move(font_face));
    return true;
}


bool Theme::load_font_face(const char* file_path, int face_index)
{
    return impl_load_font_face(m_font, file_path, face_index);
}


bool Theme::load_icon_font_face(const char* file_path, int face_index)
{
    return impl_load_font_face(m_icon_font, file_path, face_index);
}


void Theme::set_icon_codepoint(IconId icon_id, text::CodePoint codepoint)
{
    m_icon_map[static_cast<size_t>(icon_id)] = codepoint;
}


text::CodePoint Theme::icon_codepoint(IconId icon_id)
{
    return m_icon_map[static_cast<size_t>(icon_id)];
}


void Theme::set_color(ColorId color_id, graphics::Color color)
{
    m_color_map[static_cast<size_t>(color_id)] = color;
}


graphics::Color Theme::color(ColorId color_id)
{
    return m_color_map[static_cast<size_t>(color_id)];
}


}} // namespace xci::widgets
