// Font.cpp created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Font.h"

#include <xci/text/FontLibrary.h>
#include <xci/text/FontTexture.h>
#include <xci/core/log.h>

namespace xci::text {

using namespace xci::core;


// ctor+dtor have to be implemented in cpp file
// to allow use of forward declaration in unique_ptr<FontTexture>
Font::Font(Renderer& renderer, uint32_t texture_size)
    : m_renderer(renderer), m_texture_size(texture_size) {}
Font::~Font() = default;


void Font::add_face(std::unique_ptr<FontFace> face)
{
    if (!m_texture) {
        bool color = face->has_color();
        uint32_t size = std::min(m_texture_size, m_renderer.max_image_dimension_2d());
        m_texture = std::make_unique<FontTexture>(m_renderer, size, color);
    }
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


bool Font::set_style(FontStyle style)
{
    auto select_face_idx = [this](size_t idx) {
        if (idx != m_current_face) {
            m_current_face = idx;
            // Apply attributes to new face
            face().set_size(m_size);
            face().set_stroke(m_stroke_type, m_stroke_radius);
        }
    };

    // find face index by style flags
    size_t face_idx = 0;
    for (auto& face : m_faces) {
        // It's important to first try setting variable style,
        // because the reported style is incomplete.
        // E.g. "Thin" face is reported as "Regular"
        if (face->set_style(style) || face->style() == style) {
            select_face_idx(face_idx);
            return true;
        }
        ++face_idx;
    }

    // Style not found, selected the first one
    log::warning("Requested font style not found: {}", int(style));
    select_face_idx(0);
    return false;
}


bool Font::set_weight(uint16_t weight)
{
    auto select_face_idx = [this](size_t idx) {
        if (idx != m_current_face) {
            m_current_face = idx;
            // Apply attributes to new face
            face().set_size(m_size);
            face().set_stroke(m_stroke_type, m_stroke_radius);
        }
    };

    // find face index by weight and current style (e.g. italic)
    size_t face_idx = 0;
    auto cur_style = face().style();
    for (auto& face : m_faces) {
        if (face->style() == cur_style && face->weight() == weight) {
            select_face_idx(face_idx);
            return true;
        }
        ++face_idx;
    }

    // variable fonts - set 'wght' axis
    if (face().set_weight(weight))
        return true;

    // Weight not found, selected the first one
    log::warning("Requested font weight not found: {}", weight);
    return false;
}


bool Font::set_size(unsigned size)
{
    m_size = size;
    return face().set_size(m_size);
}


bool Font::set_stroke(StrokeType type, float radius)
{
    m_stroke_type = type;
    m_stroke_radius = (type == StrokeType::None) ? 0.f : radius;
    return face().set_stroke(m_stroke_type, m_stroke_radius);
}


Font::Glyph* Font::get_glyph(GlyphIndex glyph_index)
{
    // check cache
    GlyphKey glyph_key {
        m_current_face, face().size_key(), face().weight(),
        glyph_index,
        m_stroke_type, m_stroke_radius
    };
    auto iter = m_glyphs.find(glyph_key);
    if (iter != m_glyphs.end())
        return &iter->second;

    // render
    FontFace::Glyph glyph_render;
    if (!face().render_glyph(glyph_index, glyph_render))
        return nullptr;

    // insert into texture
    Glyph glyph;
    if (!m_texture->add_glyph(glyph_render.bitmap_size, glyph_render.bitmap_buffer,
                              glyph.m_tex_coords)) {
        // no more space in texture -> reset and try again
        clear_cache();
        return get_glyph(glyph_index);
    }

    // fill metrics
    glyph.m_bearing = glyph_render.bearing;
    glyph.m_advance = glyph_render.advance;

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
