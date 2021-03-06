// Font.h created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_FONT_H
#define XCI_TEXT_FONT_H

#include <xci/text/FontFace.h>
#include <xci/graphics/Renderer.h>
#include <xci/graphics/Texture.h>
#include <xci/core/geometry.h>
#include <xci/core/Vfs.h>

#include <vector>
#include <map>
#include <cassert>

namespace xci::text {

using core::Rect_u;
using graphics::Renderer;
using graphics::Texture;


class FontTexture;


// Encapsulates faces, styles and glyph caches for a font
class Font {
public:
    explicit Font(Renderer& renderer);
    ~Font();

    // non-copyable
    Font(const Font&) = delete;
    Font& operator =(const Font&) = delete;

    // Add a face. Call multiple times to add different strokes
    // (either from separate files or using face_index).
    void add_face(std::unique_ptr<FontFace> face);

    // The same as above, but constructs FontFace object for you
    // (using default FontLibrary).
    // Returns false when FontFace load operation fails.
    bool add_face(const core::Vfs& vfs, std::string path, int face_index);

    // Get currently selected face.
    FontFace& face() { check_face(); return *m_faces[m_current_face].get(); }
    const FontFace& face() const { check_face(); return *m_faces[m_current_face].get(); }

    // Select a loaded face by style
    void set_style(FontStyle style);

    // Select a size for current face. This may create a new texture (glyph table).
    void set_size(unsigned size);
    unsigned size() const { return m_size; }

    struct GlyphKey {
        size_t font_face;
        unsigned font_size;
        GlyphIndex glyph_index;

        bool operator<(const GlyphKey& rhs) const
        {
            return std::tie(font_face, font_size, glyph_index) <
                   std::tie(rhs.font_face, rhs.font_size, rhs.glyph_index);
        }
    };

    class Glyph {
    public:
        core::Vec2u size() const { return m_tex_coords.size(); }
        const core::Vec2i& bearing() const { return m_bearing; }
        float advance() const { return m_advance; }

        const Rect_u& tex_coords() const { return m_tex_coords; };

    private:
        Rect_u m_tex_coords;
        core::Vec2i m_bearing;  // FT bitmap_left, bitmap_top
        float m_advance = 0;

        friend class Font;
    };
    Glyph* get_glyph(CodePoint code_point);

    // just a facade
    float line_height() const { return face().line_height(); }
    float max_advance() { return face().max_advance(); }
    float ascender() const { return face().ascender(); }
    float descender() const { return face().descender(); }
    Texture& texture();

    // Throw away any rendered glyphs
    void clear_cache();

private:
    // check that at leas one face is loaded
    void check_face() const { assert(!m_faces.empty());  }

private:
    Renderer& m_renderer;
    unsigned m_size = 10;
    size_t m_current_face = 0;
    std::vector<std::unique_ptr<FontFace>> m_faces;  // faces for different strokes (eg. normal, bold, italic)
    std::unique_ptr<FontTexture> m_texture;  // glyph tables for different styles (size, outline)
    std::map<GlyphKey, Glyph> m_glyphs;
};


} // namespace xci::text

#endif // XCI_TEXT_FONT_H
