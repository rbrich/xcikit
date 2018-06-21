// Font.cpp created on 2018-03-02, part of XCI toolkit
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

#include "Font.h"

#include <xci/text/FontTexture.h>
#include <xci/util/log.h>

namespace xci {
namespace text {

using namespace util::log;


static inline float ft_to_float(FT_F26Dot6 ft_units) {
    return (float)(ft_units) / 64.f;
}


Font::Font() : m_texture(std::make_unique<FontTexture>()) {}
Font::~Font() = default;


void Font::add_face(FontFace &face)
{
    m_faces.emplace_back(&face);
    if (m_current_face == nullptr)
        m_current_face = m_faces.back();
}


void Font::set_size(unsigned size)
{
    m_size = size;
    if (m_current_face)
        m_current_face->set_size(m_size);
}


Font::Glyph* Font::get_glyph(CodePoint code_point)
{
    assert(m_current_face != nullptr);  // font must be loaded

    // translate char to glyph
    uint glyph_index = m_current_face->get_glyph_index(code_point);
    if (!glyph_index) {
        // Note that this is not an error, not even warning!
        // Glyph nr. 0 actually contains graphic for "undefined character code"
        log_debug("Font does not define char: {}", code_point);
    }

    // check cache
    GlyphKey glyph_key{m_current_face, m_size, glyph_index};
    auto iter = m_glyphs.find(glyph_key);
    if (iter != m_glyphs.end())
        return &iter->second;

    // render
    m_current_face->set_size(m_size);
    auto glyph_slot = m_current_face->load_glyph(glyph_index);
    if (glyph_slot == nullptr)
        return nullptr;
    auto& bitmap = m_current_face->render_glyph();

    // insert into texture
    Glyph glyph(*this);
    if (!m_texture->add_glyph(bitmap, glyph.m_tex_coords)) {
        // no more space in texture -> reset and try again
        clear_cache();
        return get_glyph(code_point);
    }

    // fill metrics
    glyph.m_base.x = glyph_slot->bitmap_left;  // ft_to_float(gm.horiBearingX)
    glyph.m_base.y = glyph_slot->bitmap_top;  // ft_to_float(gm.horiBearingY)
    glyph.m_advance = ft_to_float(glyph_slot->advance.x);

    // insert into cache
    auto glyph_item = m_glyphs.emplace(glyph_key, glyph);
    assert(glyph_item.second);
    return &glyph_item.first->second;
}

void Font::clear_cache()
{
    m_glyphs.clear();
    m_texture->clear();
}

float Font::line_height() const
{
    assert(m_current_face != nullptr);  // font must be loaded
    return m_current_face->line_height();
}


float Font::ascender() const
{
    assert(m_current_face != nullptr);  // font must be loaded
    return m_current_face->ascender();
}


float Font::descender() const
{
    assert(m_current_face != nullptr);  // font must be loaded
    return m_current_face->descender();
}


graphics::TexturePtr& Font::get_texture()
{
    return m_texture->get_texture();
}


}} // namespace xci::text
