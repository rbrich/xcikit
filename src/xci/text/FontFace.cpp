// FontFace.cpp created on 2018-09-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "FontFace.h"
#include "FontLibrary.h"
#include <xci/core/log.h>
#include <xci/compat/endian.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_MULTIPLE_MASTERS_H
#include FT_SFNT_NAMES_H

#include <hb.h>
#include <hb-ft.h>

#include <bit>
#include <cassert>

namespace xci::text {

using namespace xci::core;


#define FT_CHECK_OR(err, msg, or_stmt)  do {       \
    if (err != 0) {                             \
        log::error("{}: {}", (msg), err);       \
        or_stmt;                              \
    }                                           \
} while(false)
#define FT_CHECK_RETURN_FALSE(err, msg)  FT_CHECK_OR(err, msg, return false)

static inline float ft_to_float(FT_F26Dot6 ft_units) {
    return (float)(ft_units) / 64.f;
}

static inline FT_F26Dot6 float_to_ft(float units) {
    return (FT_F26Dot6) roundf(units * 64.f);
}


FontFace::~FontFace()
{
    if (m_hb_font != nullptr)
        hb_font_destroy(m_hb_font);

    if (m_face != nullptr) {
        auto err = FT_Done_Face(m_face);
        FT_CHECK_OR(err, "FT_Done_Face", return);
    }
    FT_Stroker_Done(m_stroker);
}


bool FontFace::load_from_file(const fs::path& file_path, int face_index)
{
    return load_face(file_path, nullptr, 0, face_index);
}


bool FontFace::load_from_memory(core::BufferPtr buffer, int face_index)
{
    m_memory_buffer = std::move(buffer);
    return load_face({}, m_memory_buffer->data(), m_memory_buffer->size(), face_index);
}


bool FontFace::set_size(unsigned pixel_size)
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
        FT_CHECK_RETURN_FALSE(err, "FT_Select_Size");
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
    FT_CHECK_RETURN_FALSE(err, "FT_Request_Size");

    hb_ft_font_set_load_flags(m_hb_font, get_load_flags());
    hb_ft_font_changed(m_hb_font);

    return true;
}


bool FontFace::set_stroke(StrokeType type, float radius)
{
    if (has_color())
        return false;

    if (type == StrokeType::None) {
        m_stroke_type = type;
        return true;
    }

    if (m_stroker == nullptr) {
        // Create stroker
        auto err = FT_Stroker_New(m_library->ft_library(), &m_stroker);
        FT_CHECK_RETURN_FALSE(err, "FT_Stroker_New");
    }

    FT_Stroker_Set(m_stroker, float_to_ft(radius),
            FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

    m_stroke_type = type;
    return true;
}


bool FontFace::has_color() const
{
    return FT_HAS_COLOR(m_face);
}


FontStyle FontFace::style() const
{
    assert(m_face != nullptr);
    static_assert(FT_STYLE_FLAG_ITALIC == int(FontStyle::Italic), "freetype italic flag == 1");
    static_assert(FT_STYLE_FLAG_BOLD == int(FontStyle::Bold), "freetype bold flag == 2");
    return static_cast<FontStyle>(m_face->style_flags & 0b11);
}


bool FontFace::FontFace::is_variable() const
{
    return (m_face->face_flags & FT_FACE_FLAG_MULTIPLE_MASTERS) != 0;
}


auto FontFace::get_variable_axes() const -> std::vector<VarAxis>
{
    FT_MM_Var* mm_var;
    auto err = FT_Get_MM_Var(m_face, &mm_var);
    FT_CHECK_OR(err, "FT_Get_MM_Var", return {});
    std::vector<VarAxis> axes;
    for (FT_UInt i = 0; i != mm_var->num_axis; ++i) {
        const auto& mm_axis = mm_var->axis[i];
        auto& ax = axes.emplace_back(VarAxis{
                .name = mm_axis.name,
                .minimum = ft_to_float(mm_axis.minimum),
                .maximum = ft_to_float(mm_axis.maximum),
                .default_ = ft_to_float(mm_axis.def),
        });
        uint32_t tag = htobe32(mm_axis.tag);
        std::memcpy(&ax.tag, &tag, sizeof ax.tag);
    }
    FT_Done_MM_Var(m_library->ft_library(), mm_var);
    return axes;
}


auto FontFace::get_variable_named_styles() const -> std::vector<VarNamedStyle>
{
    FT_MM_Var* mm_var;
    auto err = FT_Get_MM_Var(m_face, &mm_var);
    FT_CHECK_OR(err, "FT_Get_MM_Var", return {});
    std::vector<VarNamedStyle> styles;
    for (FT_UInt i = 0; i != mm_var->num_namedstyles; ++i) {
        const auto& mm_style = mm_var->namedstyle[i];
        auto& style = styles.emplace_back(VarNamedStyle{
                .name = get_name_by_strid(mm_style.strid)
        });
        style.coords.reserve(mm_var->num_axis);
        std::transform(mm_style.coords, mm_style.coords + mm_var->num_axis,
                       std::back_inserter(style.coords), ft_to_float);
    }
    FT_Done_MM_Var(m_library->ft_library(), mm_var);
    return styles;
}


bool FontFace::set_variable_axes_coords(const std::vector<float>& coords)
{
    std::vector<FT_Fixed> ft_coords;
    ft_coords.reserve(coords.size());
    std::transform(coords.begin(), coords.end(), std::back_inserter(ft_coords), float_to_ft);
    auto err = FT_Set_Var_Design_Coordinates(m_face, ft_coords.size(), ft_coords.data());
    FT_CHECK_RETURN_FALSE(err, "FT_Set_Var_Design_Coordinates");
    return true;
}


bool FontFace::set_variable_named_style(unsigned int instance_index)
{
    auto err = FT_Set_Named_Instance(m_face, instance_index);
    FT_CHECK_RETURN_FALSE(err, "FT_Set_Named_Instance");
    return true;
}


float FontFace::height() const
{
    return ft_to_float(m_face->size->metrics.height);
}


float FontFace::max_advance()
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


float FontFace::ascender() const
{
    return ft_to_float(m_face->size->metrics.ascender);
}


float FontFace::descender() const
{
    return ft_to_float(m_face->size->metrics.descender);
}


long FontFace::size_key() const
{
    return m_face->size->metrics.height;
}


GlyphIndex FontFace::get_glyph_index(CodePoint code_point) const
{
    return FT_Get_Char_Index(m_face, code_point);
}


auto FontFace::shape_text(std::string_view utf8) const -> std::vector<GlyphPlacement>
{
    hb_buffer_t *buf;
    buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, utf8.data(), (int) utf8.size(), 0, -1);

    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
    hb_buffer_set_language(buf, hb_language_from_string("en", -1));

    hb_shape(m_hb_font, buf, nullptr, 0);

    unsigned int glyph_count;
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    std::vector<GlyphPlacement> result;
    result.reserve(glyph_count);
    for (unsigned int i = 0; i < glyph_count; i++) {
        result.emplace_back(GlyphPlacement{
            .glyph_index = glyph_info[i].codepoint,
            .char_index = glyph_info[i].cluster,
            .offset = {glyph_pos[i].x_offset, glyph_pos[i].y_offset},
            .advance = {ft_to_float(glyph_pos[i].x_advance), ft_to_float(glyph_pos[i].y_advance)},
        });
    }

    hb_buffer_destroy(buf);

    return result;
}


FT_GlyphSlot FontFace::load_glyph(GlyphIndex glyph_index)
{
    int err = FT_Load_Glyph(m_face, glyph_index, get_load_flags());
    FT_CHECK_OR(err, "FT_Load_Glyph", return nullptr);
    return m_face->glyph;
}


std::string FontFace::get_name_by_strid(unsigned int strid) const
{
    auto n = FT_Get_Sfnt_Name_Count(m_face);
    for (FT_UInt i = 0; i != n; ++i) {
        FT_SfntName name_info;
        auto err = FT_Get_Sfnt_Name(m_face, i, &name_info);
        FT_CHECK_OR(err, "FT_Get_Sfnt_Name", return {});
        // FIXME: check platform_id/encoding_id/language_id
        if (name_info.name_id == strid) {
            return {(const char*)name_info.string, name_info.string_len};
        }
    }
    return {};
}


FontFace::Glyph::~Glyph()
{
    FT_Done_Glyph(ft_glyph);
}


bool FontFace::render_glyph(GlyphIndex glyph_index, Glyph& out_glyph)
{
    // render
    auto glyph_slot = load_glyph(glyph_index);
    if (glyph_slot == nullptr)
        return false;

    // Bitmap can be loaded directly by FT_LOAD_COLOR
    FT_Bitmap* bitmap = &glyph_slot->bitmap;
    if (bitmap->buffer == nullptr) {
        if (m_stroke_type != StrokeType::None) {
            assert(m_stroker != nullptr);
            FT_Glyph ft_glyph;
            FT_Error err;
            err = FT_Get_Glyph(glyph_slot, &ft_glyph);
            FT_CHECK_RETURN_FALSE(err, "FT_Get_Glyph");
            if (m_stroke_type == StrokeType::Outline) {
                err = FT_Glyph_Stroke(&ft_glyph, m_stroker, true);
                FT_CHECK_RETURN_FALSE(err, "FT_Glyph_Stroke");
            } else {
                assert(m_stroke_type == StrokeType::InsideBorder || m_stroke_type == StrokeType::OutsideBorder);
                err = FT_Glyph_StrokeBorder(&ft_glyph, m_stroker, m_stroke_type == StrokeType::InsideBorder, true);
                FT_CHECK_RETURN_FALSE(err, "FT_Glyph_StrokeBorder");
            }
            err = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, nullptr, true);
            FT_CHECK_RETURN_FALSE(err, "FT_Glyph_To_Bitmap");
            auto* bitmap_glyph = reinterpret_cast<FT_BitmapGlyph>(ft_glyph);
            bitmap = &bitmap_glyph->bitmap;
            out_glyph.bearing = {bitmap_glyph->left, bitmap_glyph->top};
            out_glyph.ft_glyph = ft_glyph;
        } else {
            auto err = FT_Render_Glyph(glyph_slot, FT_RENDER_MODE_NORMAL);
            FT_CHECK_RETURN_FALSE(err, "FT_Render_Glyph");
            out_glyph.bearing = {glyph_slot->bitmap_left, glyph_slot->bitmap_top};
        }
    } else {
        out_glyph.bearing = {glyph_slot->bitmap_left, glyph_slot->bitmap_top};
    }

    if (bitmap->width != 0) {
        // check that the bitmap is as expected
        // (this depends on FreeType settings which are under our control)
        if (bitmap->pixel_mode == FT_PIXEL_MODE_BGRA) {
            assert(has_color());
            assert((int)bitmap->width * 4 == bitmap->pitch);
        } else {
            assert(!has_color());
            assert(bitmap->pixel_mode == FT_PIXEL_MODE_GRAY);
            assert(bitmap->num_grays == 256);
            assert((int)bitmap->width == bitmap->pitch);
        }
    }

    out_glyph.bitmap_size = {bitmap->width, bitmap->rows};
    out_glyph.bitmap_buffer = bitmap->buffer;
    out_glyph.advance = {ft_to_float(glyph_slot->advance.x),
                         ft_to_float(glyph_slot->advance.y)};
    return true;
}


// Internal helper to avoid repeating error handling etc.
bool FontFace::load_face(const fs::path& file_path, const std::byte* buffer, size_t buffer_size, int face_index)
{
    if (m_face != nullptr) {
        log::error("FontFace: Reloading not supported! Create new instance instead.");
        return false;
    }
    FT_Error err;
    if (buffer) {
        err = FT_New_Memory_Face(m_library->ft_library(),
                reinterpret_cast<const FT_Byte*>(buffer), (FT_Long) buffer_size,
                face_index, &m_face);
    } else {
        err = FT_New_Face(m_library->ft_library(),
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

    m_hb_font = hb_ft_font_create_referenced(m_face);
    if (m_hb_font == nullptr) {
        log::error("hb_ft_font_create_referenced: error");
        FT_Done_Face(m_face);
        m_face = nullptr;
        return false;
    }
    hb_ft_font_set_load_flags(m_hb_font, get_load_flags());

    return true;
}


int32_t FontFace::get_load_flags() const
{
    return FT_LOAD_COLOR | (height() < 20.f ? FT_LOAD_TARGET_LIGHT : FT_LOAD_NO_HINTING);
}


} // namespace xci::text
