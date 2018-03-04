// Font.h created on 2018-03-02, part of XCI toolkit

#ifndef XCI_TEXT_FONT_H
#define XCI_TEXT_FONT_H

#include <xci/text/FontLibrary.h>
#include <xci/text/FontFace.h>
#include <xci/text/FontTexture.h>

#include <xci/util/geometry.h>
using xci::util::Vec2f;
using xci::util::Rect_u;

#include <list>
#include <map>

namespace xci {
namespace text {


// Encapsulates faces, styles and glyph caches for a font
class Font {
public:
    Font() = default;

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
    void set_size(unsigned size) { m_size = size; update_size(); }

    // Set scaling factor (forces clearing of cache)
    // Use window_size / view_size to fit the pixels exactly.
    void set_scale(float factor) { m_scale = factor; update_size(); }

    // This is inverse scale, to convert scaled size back to virtual pixels
    // Not to be used directly, scaling is handled by Glyph class.
    float get_ratio() const { return m_computed_ratio; }

    struct GlyphKey {
        FontFace* font_face;
        unsigned font_size;
        uint glyph_index;

        bool operator<(const GlyphKey& rhs) const
        {
            return std::tie(font_face, font_size, glyph_index) <
                   std::tie(rhs.font_face, rhs.font_size, rhs.glyph_index);
        }
    };

    class Glyph {
    public:
        explicit Glyph(Font& font) : m_font(font) {}

        float base_x() const { return m_base.x * m_font.get_ratio(); }
        float base_y() const { return m_base.y * m_font.get_ratio(); }
        float width() const { return m_tex_coords.w * m_font.get_ratio(); }
        float height() const { return m_tex_coords.h * m_font.get_ratio(); }
        float advance() const { return m_advance * m_font.get_ratio(); }

        float scaled_base_x() const { return m_base.x; }
        float scaled_base_y() const { return m_base.y; }
        float scaled_advance() const { return m_advance; }

        const Rect_u& tex_coords() const { return m_tex_coords; };

    private:
        Font& m_font;
        Rect_u m_tex_coords;
        Vec2f m_base;  // FT bitmap_left, bitmap_top
        float m_advance = 0;

        friend class Font;
    };
    Glyph* get_glyph(ulong code_point);

    // just a facade
    float get_line_height() const { return scaled_line_height() * m_computed_ratio; }
    float scaled_line_height() const;
    const Texture& get_texture() const { return m_texture.get_texture(); }

    // Throw away any rendered glyphs
    void clear_cache();

private:
    unsigned m_size = 10;
    float m_scale = 1.0f;
    unsigned m_computed_size = 10;
    float m_computed_ratio = 1.0;
    std::list<FontFace*> m_faces;  // faces for different strokes (eg. normal, bold, italic)
    FontFace* m_current_face = nullptr;
    FontTexture m_texture;  // glyph tables for different styles (size, outline)
    std::map<GlyphKey, Glyph> m_glyphs;

    void update_size();
};


}} // namespace xci::text

#endif // XCI_TEXT_FONT_H
