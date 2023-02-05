// Theme.cpp created on 2018-04-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Theme.h"
#include <xci/graphics/Renderer.h>

namespace xci::widgets {


#define TRY(stmt)  do { auto res = stmt; if (!res) return false; } while(0)


Theme::Theme(graphics::Renderer& renderer)
        : m_renderer(renderer),
          m_fonts{text::Font(renderer),  // base
                  text::Font(renderer, 1024u),  // emoji
                  text::Font(renderer),  // icon
                  text::Font(renderer)}  // alt
{}


bool Theme::load_default()
{
    const core::Vfs& vfs = m_renderer.vfs();

    // Base font
    TRY(load_font_face(vfs, "fonts/RobotoMono/RobotoMono_wght.ttf", 0, FontId::Base));
    TRY(load_font_face(vfs, "fonts/RobotoMono/RobotoMono-Italic_wght.ttf", 0, FontId::Base));

    // Emoji font
    TRY(load_font_face(vfs, "fonts/Noto/NotoColorEmoji.ttf", 0, FontId::Emoji));

    // Material Icons
    TRY(load_font_face(vfs, "fonts/MaterialIcons/MaterialIcons-Regular.woff", 0, FontId::Icon));
    set_icon_codepoint(IconId::None, L' ');
    set_icon_codepoint(IconId::CheckBoxUnchecked, L'\ue835');
    set_icon_codepoint(IconId::CheckBoxChecked, L'\ue834');
    set_icon_codepoint(IconId::RadioButtonUnchecked, L'\ue836');
    set_icon_codepoint(IconId::RadioButtonChecked, L'\ue837');

    // Colors
    set_color(ColorId::Default, {180, 180, 180});
    set_color(ColorId::Hover, graphics::Color::White());
    set_color(ColorId::Focus, graphics::Color::Yellow());

    return true;
}


bool Theme::load_font_face(const core::Vfs& vfs, const char* file_path, int face_index, FontId font_id)
{
    return m_fonts[size_t(font_id)].add_face(vfs, file_path, face_index);
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


} // namespace xci::widgets
