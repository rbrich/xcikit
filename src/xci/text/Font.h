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
#include <xci/util/geometry.h>

#include <list>
#include <map>

namespace xci {
namespace text {

using util::Rect_u;
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

    // Add a face. Call multiple times if the strokes are in separate files.
    void add_face(FontFace &face);

    // Select a loaded face by stroke style
    enum Stroke {
        Regular,
        Bold,
        Italic,
        BoldItalic,
    };
    void set_stroke(Stroke stroke);

    // Select a size for current face. This may create a new texture (glyph table).
    void set_size(unsigned size);
    unsigned size() const { return m_size; }

    struct GlyphKey {
        FontFace* font_face;
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
        explicit Glyph(Font& font) : m_font(font) {}

        float base_x() const { return m_base.x; }
        float base_y() const { return m_base.y; }
        float width() const { return m_tex_coords.w; }
        float height() const { return m_tex_coords.h; }
        float advance() const { return m_advance; }

        const Rect_u& tex_coords() const { return m_tex_coords; };

    private:
        Font& m_font;
        Rect_u m_tex_coords;
        util::Vec2f m_base;  // FT bitmap_left, bitmap_top
        float m_advance = 0;

        friend class Font;
    };
    Glyph* get_glyph(CodePoint code_point);

    // just a facade
    float line_height() const;
    float max_advance();
    float ascender() const;
    float descender() const;
    TexturePtr& get_texture();

    // Throw away any rendered glyphs
    void clear_cache();

private:
    unsigned m_size = 10;
    std::list<FontFace*> m_faces;  // faces for different strokes (eg. normal, bold, italic)
    FontFace* m_current_face = nullptr;
    std::unique_ptr<FontTexture> m_texture;  // glyph tables for different styles (size, outline)
    std::map<GlyphKey, Glyph> m_glyphs;
};


}} // namespace xci::text

#endif // XCI_TEXT_FONT_H
