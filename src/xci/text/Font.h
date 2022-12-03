// Font.h created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_FONT_H
#define XCI_TEXT_FONT_H

#include <xci/text/FontFace.h>
#include <xci/graphics/Renderer.h>
#include <xci/graphics/Texture.h>
#include <xci/core/mixin.h>
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


// Encapsulates the faces, styles and glyph caches for a font
class Font: private core::NonCopyable {
public:
    explicit Font(Renderer& renderer, uint32_t texture_size = 512u);
    ~Font();

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
    bool set_style(FontStyle style);

    /// Select font face by weight, or set 'wght' axis of a variable font.
    /// Common values:
    /// 100 = Thin,    200 = ExtraLight, 300 = Light
    /// 400 = Regular, 500 = Medium,     600 = SemiBold
    /// 700 = Bold,    800 = ExtraBold,  900 = Black
    /// \returns false - request could not be satisfied
    bool set_weight(uint16_t weight);

    // Select a size for current face
    bool set_size(unsigned size);
    unsigned size() const { return m_size; }

    // Select stroke type
    bool set_stroke(StrokeType type, float radius);

    struct GlyphKey {
        size_t font_face;
        long font_size;
        uint32_t font_weight;
        GlyphIndex glyph_index;
        StrokeType stroke_type;
        float stroke_radius;

        bool operator<(const GlyphKey& rhs) const {
            return std::tie(font_face, font_size, font_weight, glyph_index, stroke_type, stroke_radius)
                 < std::tie(rhs.font_face, rhs.font_size, rhs.font_weight, rhs.glyph_index,
                            rhs.stroke_type, rhs.stroke_radius);
        }
    };

    class Glyph {
    public:
        core::Vec2u size() const { return m_tex_coords.size(); }
        const core::Vec2i& bearing() const { return m_bearing; }
        float advance() const { return m_advance; }

        const Rect_u& tex_coords() const { return m_tex_coords; }

    private:
        Rect_u m_tex_coords;
        core::Vec2i m_bearing;  // FT bitmap_left, bitmap_top
        float m_advance = 0;

        friend class Font;
    };

    Glyph* get_glyph(GlyphIndex glyph_index);
    Glyph* get_glyph_for_char(CodePoint code_point) { return get_glyph(get_glyph_index(code_point)); }

    // Translate Unicode char to glyph
    // In case of failure, this returns 0, which doesn't need special handling, because
    // glyph nr. 0 contains graphic for "undefined character code".
    GlyphIndex get_glyph_index(CodePoint code_point) const { return face().get_glyph_index(code_point); }

    // Shape a text segment (e.g. a word) to a chain of placed glyphs
    std::vector<FontFace::GlyphPlacement> shape_text(std::string_view utf8) const { return face().shape_text(utf8); }

    // just a facade
    float height() const { return face().height(); }
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
    size_t m_current_face = 0;
    std::vector<std::unique_ptr<FontFace>> m_faces;  // faces for different strokes (eg. normal, bold, italic)
    std::unique_ptr<FontTexture> m_texture;  // glyph tables for different styles (size, outline)
    std::map<GlyphKey, Glyph> m_glyphs;

    uint32_t m_texture_size;
    unsigned m_size = 10;
    float m_stroke_radius = 0.f;
    StrokeType m_stroke_type = StrokeType::None;
};


} // namespace xci::text

#endif // XCI_TEXT_FONT_H
