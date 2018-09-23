// Font.h created on 2018-03-02, part of XCI toolkit
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

#ifndef XCI_TEXT_FONT_H
#define XCI_TEXT_FONT_H

#include <xci/text/FontLibrary.h>
#include <xci/text/FontFace.h>
#include <xci/graphics/Texture.h>
#include <xci/core/geometry.h>

#include <vector>
#include <map>
#include <cassert>

namespace xci {
namespace text {

using core::Rect_u;
using graphics::TexturePtr;


class FontTexture;


// Encapsulates faces, styles and glyph caches for a font
class Font {
public:
    Font();
    ~Font();

    // non-copyable
    Font(const Font&) = delete;
    Font& operator =(const Font&) = delete;

    // Add a face. Call multiple times to add differect strokes
    // (either from separate files or using face_index).
    void add_face(std::unique_ptr<FontFace> face);

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
    TexturePtr& get_texture();

    // Throw away any rendered glyphs
    void clear_cache();

private:
    // check that at leas one face is loaded
    void check_face() const { assert(!m_faces.empty());  }

private:
    unsigned m_size = 10;
    size_t m_current_face = 0;
    std::vector<std::unique_ptr<FontFace>> m_faces;  // faces for different strokes (eg. normal, bold, italic)
    std::unique_ptr<FontTexture> m_texture;  // glyph tables for different styles (size, outline)
    std::map<GlyphKey, Glyph> m_glyphs;
};


}} // namespace xci::text

#endif // XCI_TEXT_FONT_H
