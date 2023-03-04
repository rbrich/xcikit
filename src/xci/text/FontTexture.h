// FontTexture.h created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_FONTTEXTURE_H
#define XCI_TEXT_FONTTEXTURE_H

#include <rbp/MaxRectsBinPack.h>
#include <xci/graphics/Renderer.h>
#include <xci/graphics/Texture.h>
#include <xci/core/geometry/Vec2.h>

namespace xci::text {

using core::Vec2u;
using core::Rect_u;
using graphics::Texture;
using graphics::Renderer;


// Place glyphs into a texture, retrieve texture coords.

class FontTexture {
public:
    // The size is fixed. If the size request cannot be satisfied by HW,
    // smaller size will be used (HW maximum texture size).
    explicit FontTexture(Renderer& renderer,
            unsigned int size=512, bool color=false);

    // non-copyable
    FontTexture(const FontTexture&) = delete;
    FontTexture& operator =(const FontTexture&) = delete;

    /// Insert a glyph bitmap into texture, return texture coords
    /// \param size     IN size of glyph bitmap
    /// \param pixels   IN data of glyph bitmap
    /// \param coords   OUT new texture coordinates for the glyph
    /// \returns        false when there is no space
    bool add_glyph(Vec2u size, const uint8_t* pixels, Rect_u& coords);

    // Get the whole texture (cut the coords returned by `insert`
    // and you'll get your glyph picture).
    Texture& texture() { return m_texture; }

    void clear();

private:
    Renderer& m_renderer;
    Texture m_texture;
    rbp::MaxRectsBinPack m_binpack;
};

} // namespace xci::text

#endif // XCI_TEXT_FONTTEXTURE_H
