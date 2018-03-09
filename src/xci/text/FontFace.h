// FontFace.h created on 2018-03-02, part of XCI toolkit
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

#ifndef XCI_TEXT_FONTFACE_H
#define XCI_TEXT_FONTFACE_H

#include <xci/text/FontLibrary.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

namespace xci {
namespace text {


using CodePoint = uint32_t;
using GlyphIndex = uint32_t;


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

    GlyphIndex get_glyph_index(CodePoint code_point) const;

    // Returns null on error
    FT_GlyphSlot load_glyph(GlyphIndex glyph_index);

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
