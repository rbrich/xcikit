// FtFontFace.cpp created on 2018-09-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/log.h>
#include "FtFontFace.h"
#include "FtFontLibrary.h"
#include <cassert>

namespace xci::text {

using namespace xci::core;


static inline float ft_to_float(FT_F26Dot6 ft_units) {
    return (float)(ft_units) / 64.f;
}

static inline FT_F26Dot6 float_to_ft(float units) {
    return (FT_F26Dot6) roundf(units * 64.f);
}


FtFontFace::~FtFontFace()
{
    if (m_face != nullptr) {
        auto err = FT_Done_Face(m_face);
        if (err) {
            log::error("FT_Done_FreeType: {}", err);
            return;
        }
    }
    if (m_stroker != nullptr) {
        FT_Stroker_Done(m_stroker);
    }
}


bool FtFontFace::load_from_file(const fs::path& file_path, int face_index)
{
    return load_face(file_path, nullptr, 0, face_index);
}


bool FtFontFace::load_from_memory(core::BufferPtr buffer, int face_index)
{
    m_memory_buffer = std::move(buffer);
    return load_face({}, m_memory_buffer->data(), m_memory_buffer->size(), face_index);
}


bool FtFontFace::set_size(unsigned pixel_size)
{
    if (has_color()) {
        // Find the nearest size in available sizes:
        // - height bigger or equal to request
        FT_Int strike_index = 0;
        FT_Short strike_h = std::numeric_limits<FT_Short>::max();
        for (FT_Int i = 0; i != m_face->num_fixed_sizes; ++i) {
            const auto h = m_face->available_sizes[i].height;
            if (h >= (FT_Short) pixel_size && h < strike_h) {
                strike_index = i;
                strike_h = h;
            }
        }
        auto err = FT_Select_Size(m_face, strike_index);
        if (err) {
            log::error("FT_Select_Size: {}", err);
            return false;
        }
        return true;
    }

    FT_Size_RequestRec size_req = {
            .type = FT_SIZE_REQUEST_TYPE_CELL,
            .width = 0,
            .height = float_to_ft((float) pixel_size),
            .horiResolution = 0,
            .vertResolution = 0
    };
    auto err = FT_Request_Size(m_face, &size_req);
    if (err) {
        log::error("FT_Request_Size: {}", err);
        return false;
    }

    return true;
}


bool FtFontFace::set_outline()
{
    if (m_stroker == nullptr) {
        // Create stroker
        auto err = FT_Stroker_New(static_cast<FtFontLibrary*>(m_library.get())->ft_library(), &m_stroker);
        if (err) {
            log::error("FT_Stroker_New: {}", err);
            return false;
        }
    }

    // TODO

    return true;
}


FontStyle FtFontFace::style() const
{
    assert(m_face != nullptr);
    static_assert(FT_STYLE_FLAG_ITALIC == int(FontStyle::Italic), "freetype italic flag == 1");
    static_assert(FT_STYLE_FLAG_BOLD == int(FontStyle::Bold), "freetype bold flag == 2");
    return static_cast<FontStyle>(m_face->style_flags & 0b11);
}


float FtFontFace::height() const
{
    return ft_to_float(m_face->size->metrics.height);
}


float FtFontFace::max_advance()
{
    // Measure letter 'M' instead of trusting max_advance
    auto glyph_index = get_glyph_index('M');
    if (!glyph_index) {
        return ft_to_float(m_face->size->metrics.max_advance);
    }
    auto glyph_slot = load_glyph(glyph_index);
    if (glyph_slot == nullptr) {
        return ft_to_float(m_face->size->metrics.max_advance);
    }
    return ft_to_float(glyph_slot->metrics.horiAdvance);
}


float FtFontFace::ascender() const
{
    return ft_to_float(m_face->size->metrics.ascender);
}


float FtFontFace::descender() const
{
    return ft_to_float(m_face->size->metrics.descender);
}


GlyphIndex FtFontFace::get_glyph_index(CodePoint code_point) const
{
    return FT_Get_Char_Index(m_face, code_point);
}


FT_GlyphSlot FtFontFace::load_glyph(GlyphIndex glyph_index)
{
    int err = FT_Load_Glyph(m_face, glyph_index,
            FT_LOAD_COLOR | (height() < 20.f ? FT_LOAD_TARGET_LIGHT : FT_LOAD_NO_HINTING));
    if (err) {
        log::error("FT_Load_Glyph error: {}", err);
        return nullptr;
    }
    return m_face->glyph;
}


bool FtFontFace::render_glyph(GlyphIndex glyph_index, Glyph& glyph)
{
    // render
    auto glyph_slot = load_glyph(glyph_index);
    if (glyph_slot == nullptr)
        return false;

    // Bitmap can be loaded directly by FT_LOAD_COLOR
    auto& bitmap = m_face->glyph->bitmap;
    if (bitmap.buffer == nullptr) {
        int err = FT_Render_Glyph(m_face->glyph, FT_RENDER_MODE_NORMAL);
        if (err) {
            log::error("FT_Render_Glyph error: {}", err);
        }
    }

    if (bitmap.width != 0) {
        // check that the bitmap is as expected
        // (this depends on FreeType settings which are under our control)
        if (bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
            assert((int)bitmap.width * 4 == bitmap.pitch);
            glyph.bgra = true;
        } else {
            assert(bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);
            assert(bitmap.num_grays == 256);
            assert((int)bitmap.width == bitmap.pitch);
        }
    }

    glyph.bitmap_size = {bitmap.width, bitmap.rows};
    glyph.bitmap_buffer = bitmap.buffer;
    glyph.bearing = {glyph_slot->bitmap_left, glyph_slot->bitmap_top};
    glyph.advance = {ft_to_float(glyph_slot->advance.x),
                     ft_to_float(glyph_slot->advance.y)};
    return true;
}


FT_Library FtFontFace::ft_library()
{
    return static_cast<FtFontLibrary*>(m_library.get())->ft_library();
}


// Internal helper to avoid repeating error handling etc.
bool FtFontFace::load_face(const fs::path& file_path, const std::byte* buffer, size_t buffer_size, int face_index)
{
    if (m_face != nullptr) {
        log::error("FontFace: Reloading not supported! Create new instance instead.");
        return false;
    }
    FT_Error err;
    if (buffer) {
        err = FT_New_Memory_Face(ft_library(),
                reinterpret_cast<const FT_Byte*>(buffer), (FT_Long) buffer_size,
                face_index, &m_face);
    } else {
        err = FT_New_Face(ft_library(),
                file_path.string().c_str(), face_index, &m_face);
    }
    if (err == FT_Err_Unknown_File_Format) {
        log::error("FT_New_Face: Unknown file format");
        return false;
    }
    if (err == FT_Err_Cannot_Open_Resource) {
        log::error("FT_New_Face: Cannot open resource");
        return false;
    }
    if (err != 0) {
        log::error("Cannot open font (FT_New_Face: {})", err);
        return false;
    }

    // Our code points are in Unicode, make sure it's selected
    if (FT_Select_Charmap(m_face, FT_ENCODING_UNICODE) != 0) {
        log::error("FT_Select_Charmap: Error setting to Unicode: {}", err);
        FT_Done_Face(m_face);
        m_face = nullptr;
        return false;
    }

    return true;
}


} // namespace xci::text
