// FontTexture.h created on 2018-03-02, part of XCI toolkit

#ifndef XCI_TEXT_FONTTEXTURE_H
#define XCI_TEXT_FONTTEXTURE_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include <rbp/MaxRectsBinPack.h>

#include <SFML/Graphics/Texture.hpp>

namespace xci {
namespace text {

// Render glyphs into texture, then retrieve a rect for a glyph.
// Note that you need a different FontTexture for every FontFace, size and style.
// (The glyphs are searched only by glyph_index.)

class FontTexture {
public:
    // The size is fixed. If the size_request cannot be satisfied by HW,
    // smaller size will be used (HW maximum texture size).
    explicit FontTexture(unsigned int size_request=512);

    // non-copyable
    FontTexture(const FontTexture&) = delete;
    FontTexture& operator =(const FontTexture&) = delete;

    // Returns actual size of texture
    unsigned int get_size() { return m_texture.getSize().x; }

    // Insert a glyph bitmap into texture, get texture coords
    // Returns false when there is no space.
    bool add_glyph(const FT_Bitmap& bitmap, sf::IntRect& coords);

    // Get the whole texture (cut the coords returned by `insert`
    // and you'll get your glyph picture).
    const sf::Texture& get_texture() const { return m_texture; }

    void clear();

private:
    sf::Texture m_texture;
    rbp::MaxRectsBinPack m_binpack;
};

}} // namespace xci::text

#endif // XCI_TEXT_FONTTEXTURE_H
