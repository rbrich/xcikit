// FtFontFace.h created on 2018-09-23, part of XCI toolkit
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

#ifndef XCI_TEXT_FREETYPE_FONTFACE_H
#define XCI_TEXT_FREETYPE_FONTFACE_H

#include <xci/text/FontFace.h>
#include <xci/core/Buffer.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

#include <filesystem>

namespace xci::text {

namespace fs = std::filesystem;


class FtFontFace: public FontFace {
public:
    explicit FtFontFace(FontLibraryPtr library) : FontFace(std::move(library)) {}
    ~FtFontFace() override;

    bool load_from_file(const fs::path& file_path, int face_index) override;
    bool load_from_memory(core::BufferPtr buffer, int face_index) override;

    bool set_size(unsigned pixel_size) override;

    bool set_outline() override;  // TODO

    FontStyle style() const override;
    float line_height() const override;
    float max_advance() override;
    float ascender() const override;
    float descender() const override;

    GlyphIndex get_glyph_index(CodePoint code_point) const override;

    bool render_glyph(GlyphIndex glyph_index, Glyph& glyph) override;

private:
    FT_Library ft_library();
    bool load_face(const fs::path& file_path, const byte* buffer, size_t buffer_size, int face_index);

    // Returns null on error
    FT_GlyphSlot load_glyph(GlyphIndex glyph_index);

private:
    core::BufferPtr m_memory_buffer;
    FT_Face m_face = nullptr;
    FT_Stroker m_stroker = nullptr;
};


} // namespace xci::text

#endif // XCI_TEXT_FREETYPE_FONTFACE_H
