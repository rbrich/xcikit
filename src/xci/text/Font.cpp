// Font.cpp created on 2018-03-02, part of XCI toolkit

#include "Font.h"

#include <xci/util/log.h>
using namespace xci::util::log;

namespace xci {
namespace text {


static inline float ft_to_float(FT_F26Dot6 ft_units) {
    return (float)(ft_units) / 64.f;
}


void Font::add_face(FontFace &face)
{
    m_faces.emplace_back(&face);
    if (m_current_face == nullptr)
        m_current_face = m_faces.back();
}


Font::Glyph* Font::get_glyph(ulong code_point)
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
    GlyphKey glyph_key{m_current_face, m_computed_size, glyph_index};
    auto iter = m_glyphs.find(glyph_key);
    if (iter != m_glyphs.end())
        return &iter->second;

    // render
    m_current_face->set_size(m_computed_size);
    auto glyph_slot = m_current_face->load_glyph(glyph_index);
    if (glyph_slot == nullptr)
        return nullptr;
    auto& bitmap = m_current_face->render_glyph();

    // insert into texture
    Glyph glyph(*this);
    if (!m_texture.add_glyph(bitmap, glyph.m_tex_coords)) {
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
    m_texture.clear();
}

void Font::update_size()
{
    constexpr float oversample = 1.0f;  // oversample font texture against screen
    m_computed_size = unsigned(m_size * m_scale * oversample + 0.99f);
    m_computed_ratio = float(m_size) / float(m_computed_size);  // inverse scale
}

float Font::scaled_line_height() const
{
    m_current_face->set_size(m_computed_size);
    return m_current_face->get_line_height();
}


}} // namespace xci::text
