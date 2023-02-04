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

enum class FontId {
    Base,       // base font
    Emoji,      // emoji font (fallback)
    Icon,       // icons
    Alt,        // alternative font
};

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

    /// Load font face from VFS
    /// Multiple faces can be loaded into the same target FontID, e.g. Regular, Bold, Italic.
    /// \param vfs          VFS where to look for the font file
    /// \param file_path    path to font file inside the VFS
    /// \param face_index   index of font face in the font file which should be loaded (0 = default / single face)
    /// \param font_id      target FontID to which the face should be added
    bool load_font_face(const core::Vfs& vfs, const char* file_path, int face_index, FontId font_id);
    text::Font& font(FontId font_id) { return m_fonts[size_t(font_id)]; }
    text::Font& base_font() { return font(FontId::Base); }
    text::Font& emoji_font() { return font(FontId::Emoji); }
    text::Font& icon_font() { return font(FontId::Icon); }
    text::Font& alt_font() { return font(FontId::Alt); }

    // icons
    void set_icon_codepoint(IconId icon_id, text::CodePoint codepoint);
    text::CodePoint icon_codepoint(IconId icon_id);

    // colors
    void set_color(ColorId color_id, graphics::Color color);
    graphics::Color color(ColorId color_id);

private:
    graphics::Renderer& m_renderer;
    std::array<text::Font, sizeof(FontId)> m_fonts;
    IconMap m_icon_map {};
    // colors
    ColorMap m_color_map;
};


} // namespace xci::widgets

#endif // XCI_WIDGETS_THEME_H
