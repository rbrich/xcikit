// FontTexture.h created on 2018-03-02, part of XCI toolkit

#ifndef XCI_TEXT_FONTTEXTURE_H
#define XCI_TEXT_FONTTEXTURE_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include <rbp/MaxRectsBinPack.h>
#include <xci/graphics/Texture.h>
#include <xci/util/geometry.h>

namespace xci {
namespace text {

// Places glyphs into a texture

class FontTexture {
public:
    // The size is fixed. If the size request cannot be satisfied by HW,
    // smaller size will be used (HW maximum texture size).
    explicit FontTexture(unsigned int size=512);

    // non-copyable
    FontTexture(const FontTexture&) = delete;
    FontTexture& operator =(const FontTexture&) = delete;

    // Returns actual size of texture
    unsigned int get_size() { return m_texture.height(); }

    // Insert a glyph bitmap into texture, get texture coords
    // Returns false when there is no space.
    bool add_glyph(const FT_Bitmap& bitmap, Rect_u& coords);

    // Get the whole texture (cut the coords returned by `insert`
    // and you'll get your glyph picture).
    const graphics::Texture& get_texture() const { return m_texture; }

    void clear();

private:
    graphics::Texture m_texture;
    rbp::MaxRectsBinPack m_binpack;
};

}} // namespace xci::text

#endif // XCI_TEXT_FONTTEXTURE_H
