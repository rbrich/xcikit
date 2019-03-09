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

#include <xci/text/FontLibrary.h>
#include <xci/text/FontTexture.h>
#include <xci/core/Vfs.h>
#include <xci/core/log.h>

namespace xci {
namespace text {

using namespace core::log;
using core::Vfs;


// dtor has to be implemented in cpp file to allow forward declaration of unique_ptr<FontTexture>
Font::Font() = default;
Font::~Font() = default;


void Font::add_face(std::unique_ptr<FontFace> face)
{
    if (!m_texture)
        m_texture = std::make_unique<FontTexture>();
    m_faces.emplace_back(std::move(face));
}


bool Font::add_face(std::string path, int face_index)
{
    auto face = FontLibrary::default_instance()->create_font_face();
    auto face_file = Vfs::default_instance().read_file(std::move(path));
    if (face_file.is_real_file()) {
        // it's a real file, use only the path, let FreeType read the data
        if (!face->load_from_file(face_file.path(), face_index))
            return false;
    } else {
        // not real file, we have to read all data into memory
        if (!face->load_from_memory(face_file.content(), face_index))
            return false;
    }
    add_face(std::move(face));
    return true;
}


void Font::set_style(FontStyle style)
{
    m_current_face = 0;
    for (auto& face : m_faces) {
        if (face->style() == style) {
            return;
        }
        ++m_current_face;
    }
    // Style not found, selected the first one
    log_warning("Requested font style not found: {}", int(style));
}


void Font::set_size(unsigned size)
{
    m_size = size;
    face().set_size(m_size);
}

Font::Glyph* Font::get_glyph(CodePoint code_point)
{
    // translate char to glyph
    // In case of failure, this returns 0, which is okay, because
    // glyph nr. 0 contains graphic for "undefined character code".
    uint glyph_index = face().get_glyph_index(code_point);

    // check cache
    GlyphKey glyph_key{m_current_face, m_size, glyph_index};
    auto iter = m_glyphs.find(glyph_key);
    if (iter != m_glyphs.end())
        return &iter->second;

    // render
    face().set_size(m_size);
    FontFace::Glyph glyph_render;
    if (!face().render_glyph(glyph_index, glyph_render)) {
        return nullptr;
    }

    // insert into texture
    Glyph glyph;
    if (!m_texture->add_glyph(glyph_render.bitmap_size, glyph_render.bitmap_buffer,
                              glyph.m_tex_coords)) {
        // no more space in texture -> reset and try again
        clear_cache();
        return get_glyph(code_point);
    }

    // fill metrics
    glyph.m_bearing = glyph_render.bearing;
    glyph.m_advance = glyph_render.advance.x;

    // insert into cache
    auto glyph_item = m_glyphs.emplace(glyph_key, glyph);
    assert(glyph_item.second);
    return &glyph_item.first->second;
}


void Font::clear_cache()
{
    m_glyphs.clear();
    if (m_texture)
        m_texture->clear();
}


TexturePtr& Font::get_texture()
{
    return m_texture->get_texture();
}


}} // namespace xci::text
