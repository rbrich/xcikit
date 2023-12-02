// FontTexture.cpp created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/text/FontTexture.h>

#include <xci/core/log.h>

namespace xci::text {

using namespace core::log;
using graphics::ColorFormat;


FontTexture::FontTexture(Renderer& renderer, unsigned int size, bool color)
    : m_renderer(renderer),
      m_texture(m_renderer, color ? ColorFormat::BGRA : ColorFormat::Grey)
{
    if (!m_texture.create({size, size}))
        throw std::runtime_error("Could not create font texture.");
    m_binpack.Init(int(size), int(size), false);
    m_texture.clear();
}


bool FontTexture::add_glyph(Vec2u size, const uint8_t* pixels, Rect_u& coords)
{
    // empty bitmap -> zero coords
    if (size.x == 0 || size.y == 0) {
        coords = {0, 0, 0, 0};
        return true;
    }

    // try to place the rect
    constexpr int p = 1;  // padding
    constexpr int pp = 2 * p;
    rbp::Rect rect = m_binpack.Insert(int(size.x) + pp, int(size.y) + pp,
                                      rbp::MaxRectsBinPack::RectBestShortSideFit);
    if (rect.height == 0 || rect.width == 0)
        return false;
    assert(rect.width > 0);
    assert(rect.height > 0);
    assert(rect.x >= 0);
    assert(rect.y >= 0);

    // set output coords
    coords = {unsigned(rect.x + p), unsigned(rect.y + p), size.x, size.y};

    // copy pixels into texture
    m_texture.write(pixels, coords);
    return true;
}


void FontTexture::clear()
{
    auto ts = m_texture.size();
    m_binpack.Init(int(ts.x), int(ts.y), false);
    m_texture.clear();
}


} // namespace xci::text
