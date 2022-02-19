// FontFace.h created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_FONTFACE_H
#define XCI_TEXT_FONTFACE_H

#include <xci/text/FontLibrary.h>
#include <xci/core/geometry.h>
#include <xci/core/Buffer.h>
#include <xci/core/mixin.h>

#include <memory>  // shared_ptr
#include <vector>
#include <string>
#include <string_view>
#include <filesystem>

// Forward decls from <ft2build.h>, <hb.h>
// (to avoid external dependency on Freetype)
typedef struct FT_FaceRec_*  FT_Face;
typedef struct FT_StrokerRec_*  FT_Stroker;
typedef struct FT_GlyphSlotRec_*  FT_GlyphSlot;
typedef struct FT_GlyphRec_*  FT_Glyph;
typedef struct hb_font_t hb_font_t;


namespace xci::text {


using CodePoint = char32_t;
using GlyphIndex = uint32_t;

namespace fs = std::filesystem;


// This enum can also be considered a bitset,
// with 0b01 = italic, 0b10 = bold.
enum class FontStyle {
    Regular,        // 00
    Italic,         // 01
    Bold,           // 10
    BoldItalic,     // 11
};


enum class StrokeType {
    None,           // normal filled rendering
    Outline,        // outlined glyphs
    InsideBorder,   // inside border glyphs (for multi-pass rendering)
    OutsideBorder,  // outside border glyphs (for multi-pass rendering)
};


// Wrapper around FT_Face. Set size and attributes,
// retrieve rendered glyphs (bitmaps) and glyph metrics.

class FontFace: private core::NonCopyable {
public:
    explicit FontFace(FontLibraryPtr library) : m_library(std::move(library)) {}
    ~FontFace();

    bool load_from_file(const fs::path& file_path, int face_index);
    bool load_from_memory(core::BufferPtr buffer, int face_index);

    bool set_size(unsigned pixel_size);

    bool set_stroke(StrokeType type, float radius);

    bool has_color() const;
    FontStyle style() const;

    // Variable fonts
    bool is_variable() const;

    struct VarAxis {
        std::string name;   // e.g. "Weight"
        char tag[4];        // e.g. "wght"
        float minimum;
        float maximum;
        float default_;
    };
    std::vector<VarAxis> get_variable_axes() const;

    struct VarNamedStyle {
        std::vector<float> coords;
        std::string name;
    };
    std::vector<VarNamedStyle> get_variable_named_styles() const;

    bool set_variable_axes_coords(const std::vector<float>& coords);

    /// Select one of named styles (index starting with 1).
    /// Set 0 to reset to default style.
    bool set_variable_named_style(unsigned int instance_index);

    // Font metrics
    float height() const;
    float max_advance();
    float ascender() const;
    float descender() const;

    // Font size in internal units
    // Use only as opaque key for caching.
    long size_key() const;

    GlyphIndex get_glyph_index(CodePoint code_point) const;

    struct GlyphPlacement {
        GlyphIndex glyph_index;
        uint32_t char_index;
        core::Vec2i offset;
        core::Vec2f advance;
    };
    std::vector<GlyphPlacement> shape_text(std::string_view utf8) const;

    struct Glyph {
        core::Vec2i bearing;
        core::Vec2f advance;
        core::Vec2u bitmap_size;
        uint8_t* bitmap_buffer = nullptr;
        // Owned Freetype handle
        FT_Glyph ft_glyph = nullptr;
        ~Glyph();
    };
    bool render_glyph(GlyphIndex glyph_index, Glyph& out_glyph);

private:
    bool load_face(const fs::path& file_path, const std::byte* buffer, size_t buffer_size, int face_index);

    int32_t get_load_flags() const;

    // Returns null on error
    FT_GlyphSlot load_glyph(GlyphIndex glyph_index);

    std::string get_name_by_strid(unsigned int strid) const;

    // private data
    std::shared_ptr<FontLibrary> m_library;
    core::BufferPtr m_memory_buffer;
    FT_Face m_face = nullptr;
    FT_Stroker m_stroker = nullptr;
    hb_font_t* m_hb_font = nullptr;
    StrokeType m_stroke_type = StrokeType::None;
};


} // namespace xci::text

#endif // XCI_TEXT_FONTFACE_H
