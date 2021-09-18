// Font.cpp created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Font.h"

#include <xci/text/FontLibrary.h>
#include <xci/text/FontTexture.h>
#include <xci/core/log.h>

namespace xci::text {

using namespace xci::core;


// ctor+dtor have to be implemented in cpp file
// to allow use of forward declaration in unique_ptr<FontTexture>
Font::Font(Renderer& renderer) : m_renderer(renderer) {}
Font::~Font() = default;


void Font::add_face(std::unique_ptr<FontFace> face)
{
    if (!m_texture)
        m_texture = std::make_unique<FontTexture>(m_renderer);
    m_faces.emplace_back(std::move(face));
}


bool Font::add_face(const core::Vfs& vfs, std::string path, int face_index)
{
    auto face = FontLibrary::default_instance()->create_font_face();
    auto face_file = vfs.read_file(std::move(path));
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
    log::warning("Requested font style not found: {}", int(style));
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
    auto glyph_index = face().get_glyph_index(code_point);

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


Texture& Font::texture()
{
    return m_texture->texture();
}


} // namespace xci::text
