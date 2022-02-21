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

namespace fs = std::filesystem;

using CodePoint = char32_t;
using GlyphIndex = uint32_t;

class FontFace;


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


/// Extracts and contains information about font variations ("variable fonts")
///
/// Reference:
/// - https://freetype.org/freetype2/docs/reference/ft2-multiple_masters.html
/// - https://docs.microsoft.com/en-us/typography/opentype/spec/fvar
/// - https://web.dev/variable-fonts/
class FontVar: private core::NonCopyable {
public:
    struct Axis {
        std::string name;   // e.g. "Weight"
        uint32_t tag;       // e.g. "wght"
        float minimum;
        float maximum;
        float default_;
        float current;
    };

    struct NamedStyle {
        std::vector<float> coords;
        std::string name;
    };

    explicit FontVar(const FontFace& font_face);

    const std::vector<Axis>& axes() const { return m_axes; }
    const std::vector<NamedStyle>& named_styles() const { return m_named_styles; }

    // -1 = not available
    int8_t weight_coord_index() const;

    /// Decode 32-bit tag to 4-byte string
    static std::string decode_tag(uint32_t tag);

    /// Make 32-bit tag (as used in Axis) from 4-byte string
    static constexpr uint32_t make_tag(const char name[4]);

private:
    std::vector<Axis> m_axes;
    std::vector<NamedStyle> m_named_styles;
};


/// Wrapper around FT_Face. Set size and attributes,
/// retrieve rendered glyphs (bitmaps) and glyph metrics.
class FontFace: private core::NonCopyable {
public:
    explicit FontFace(FontLibraryPtr library) : m_library(std::move(library)) {}
    ~FontFace();

    bool load_from_file(const fs::path& file_path, int face_index);
    bool load_from_memory(core::BufferPtr buffer, int face_index);

    bool set_size(unsigned pixel_size);

    bool set_stroke(StrokeType type, float radius);

    bool has_color() const;

    /// Style can be set on variable fonts
    /// \returns true if the style was set (when supported by the face)
    bool set_style(FontStyle style);
    /// Query style of the face, or style of the current variation
    FontStyle style() const;

    /// Weight can be set on variable fonts
    /// \returns true if the weight was set (when supported by the face)
    bool set_weight(uint16_t weight);
    /// Query weight of the face, or weight of the current variation
    uint16_t weight() const;

    // -------------------------------------------------------------------------
    // Variable fonts

    bool is_variable() const;

    /// Get information about variable axes and named styles
    FontVar get_variable() const { return FontVar(*this); }

    /// Get current values of variable axes coords
    std::vector<float> get_variable_axes_coords() const;

    /// Set variable axes coords to new values
    bool set_variable_axes_coords(const std::vector<float>& coords);

    /// Select one of named styles (index starting with 1).
    /// Set 0 to reset to default style.
    bool set_variable_named_style(unsigned int instance_index);

    // -------------------------------------------------------------------------
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

    FT_Face ft_face() const { return m_face; }
    FT_Library ft_library() const { return m_library->ft_library(); }

private:
    bool load_face(const fs::path& file_path, const std::byte* buffer, size_t buffer_size, int face_index);

    int32_t get_load_flags() const;

    // Returns null on error
    FT_GlyphSlot load_glyph(GlyphIndex glyph_index);

    // private data
    std::shared_ptr<FontLibrary> m_library;
    core::BufferPtr m_memory_buffer;
    FT_Face m_face = nullptr;
    FT_Stroker m_stroker = nullptr;
    hb_font_t* m_hb_font = nullptr;
    StrokeType m_stroke_type = StrokeType::None;

    static constexpr int8_t c_var_uninitialized = -2;
    static constexpr int8_t c_var_not_available = -1;
    mutable int8_t m_var_weight = c_var_uninitialized;  // index of variable coord for weight
};


} // namespace xci::text

#endif // XCI_TEXT_FONTFACE_H
