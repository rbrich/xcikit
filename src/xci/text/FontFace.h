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
#include <xci/core/geometry.h>
#include <xci/core/Buffer.h>
#include <xci/compat/string_view.h>

#include <memory>  // shared_ptr
#include <vector>
#include <string>

namespace xci::text {


using CodePoint = char32_t;
using GlyphIndex = uint32_t;


// This enum can also be considered a bitset,
// with 0b01 = italic, 0b10 = bold.
enum class FontStyle {
    Regular,        // 00
    Italic,         // 01
    Bold,           // 10
    BoldItalic,     // 11
};


// Wrapper around FT_Face. Set size and attributes,
// retrieve rendered glyphs (bitmaps) and glyph metrics.

class FontFace {
public:
    explicit FontFace(FontLibraryPtr library) : m_library(std::move(library)) {}
    virtual ~FontFace() = default;

    // non-copyable
    FontFace(const FontFace&) = delete;
    FontFace& operator =(const FontFace&) = delete;

    virtual bool load_from_file(string_view file_path, int face_index) = 0;
    virtual bool load_from_memory(core::BufferPtr buffer, int face_index) = 0;

    virtual bool set_size(unsigned pixel_size) = 0;

    virtual bool set_outline() = 0;

    virtual FontStyle style() const = 0;
    virtual float line_height() const = 0;
    virtual float max_advance() = 0;
    virtual float ascender() const = 0;
    virtual float descender() const = 0;

    virtual GlyphIndex get_glyph_index(CodePoint code_point) const = 0;

    struct Glyph {
        core::Vec2u bitmap_size;
        uint8_t* bitmap_buffer = nullptr;
        core::Vec2i bearing;
        core::Vec2f advance;
    };
    virtual bool render_glyph(GlyphIndex glyph_index, Glyph& glyph) = 0;

protected:
    std::shared_ptr<FontLibrary> m_library;
};


} // namespace xci::text

#endif // XCI_TEXT_FONTFACE_H
