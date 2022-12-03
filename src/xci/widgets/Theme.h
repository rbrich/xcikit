// Theme.h created on 2018-04-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

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

    NumItems_,
};

constexpr size_t IconMapSize = static_cast<size_t>(IconId::NumItems_);
using IconMap = std::array<text::CodePoint, IconMapSize>;


enum class ColorId {
    Default,
    Hover,
    Focus,

    NumItems_,
};

constexpr size_t ColorMapSize = static_cast<size_t>(ColorId::NumItems_);
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

    // emoji font (fallback)
    bool load_emoji_font_face(const core::Vfs& vfs, const char* file_path, int face_index);
    text::Font& emoji_font() { return m_emoji_font; }

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
    text::Font m_font;  // base font
    text::Font m_emoji_font;  // emojis
    text::Font m_icon_font;  // icons
    IconMap m_icon_map {};
    // colors
    ColorMap m_color_map;
};


} // namespace xci::widgets

#endif // XCI_WIDGETS_THEME_H
