// FtFontFace.h created on 2018-09-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_FREETYPE_FONTFACE_H
#define XCI_TEXT_FREETYPE_FONTFACE_H

#include <xci/text/FontFace.h>
#include <xci/core/Buffer.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

#include <hb.h>
#include <hb-ft.h>

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

    bool has_color() const override { return FT_HAS_COLOR(m_face); }
    FontStyle style() const override;

    float height() const override;
    float max_advance() override;
    float ascender() const override;
    float descender() const override;

    long size_key() const override { return m_face->size->metrics.height; }

    GlyphIndex get_glyph_index(CodePoint code_point) const override;

    std::vector<GlyphPlacement> shape_text(std::string_view utf8) const override;

    bool render_glyph(GlyphIndex glyph_index, Glyph& glyph) override;

private:
    FT_Library ft_library();
    bool load_face(const fs::path& file_path, const std::byte* buffer, size_t buffer_size, int face_index);

    FT_Int32 get_load_flags() const {
        return FT_LOAD_COLOR | (height() < 20.f ? FT_LOAD_TARGET_LIGHT : FT_LOAD_NO_HINTING);
    }

    // Returns null on error
    FT_GlyphSlot load_glyph(GlyphIndex glyph_index);

private:
    core::BufferPtr m_memory_buffer;
    FT_Face m_face = nullptr;
    FT_Stroker m_stroker = nullptr;
    hb_font_t* m_hb_font = nullptr;
};


} // namespace xci::text

#endif // XCI_TEXT_FREETYPE_FONTFACE_H
