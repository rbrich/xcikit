// FontLibrary.h created on 2018-03-02, part of XCI toolkit

#ifndef XCI_TEXT_FONTFACE_H
#define XCI_TEXT_FONTFACE_H

#include <xci/text/FontLibrary.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

namespace xci {
namespace text {

// Wrapper around FT_Face. Set size and attributes,
// retrieve rendered glyphs (bitmaps) and glyph metrics.

class FontFace {
public:
    FontFace() : library(FontLibrary::get_default_instance()) {}
    explicit FontFace(std::shared_ptr<FontLibrary> library) : library(std::move(library)) {}
    ~FontFace();

    // non-copyable
    FontFace(const FontFace&) = delete;
    FontFace& operator =(const FontFace&) = delete;

    bool load_from_file(const char* file_path, int face_index);
    bool load_from_memory(const uint8_t* buffer, ssize_t size, int face_index);

    bool set_size(unsigned pixel_size);

    bool set_outline();

    float get_line_height() const;

    unsigned int get_glyph_index(unsigned long code_point) const;

    // Returns null on error
    FT_GlyphSlot load_glyph(unsigned int glyph_index);

    FT_Glyph_Metrics& get_glyph_metrics();

    FT_Bitmap& render_glyph();

private:
    template<typename F> bool load_face(F load_fn);

    std::shared_ptr<FontLibrary> library;
    FT_Face face = nullptr;
    FT_Stroker stroker = nullptr;
};

}} // namespace xci::text

#endif // XCI_TEXT_FONTFACE_H
