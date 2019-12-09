// FontTexture.cpp created on 2018-03-02, part of XCI toolkit
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

#include <xci/text/FontTexture.h>

#include <xci/core/log.h>

namespace xci::text {

using namespace core::log;


FontTexture::FontTexture(Renderer& renderer, unsigned int size)
    : m_renderer(renderer),
      m_texture(m_renderer)
{
    if (!m_texture.create({size, size}))
        throw std::runtime_error("Could not create font texture.");
    //m_texture.setSmooth(true);
    m_binpack.Init(int(size), int(size), /*allowFlip=*/false);
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
    rbp::Rect rect = m_binpack.Insert(size.x + pp, size.y + pp,
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
    m_texture.update(pixels, coords);
    return true;
}


void FontTexture::clear()
{
    auto ts = m_texture.size();
    m_binpack.Init(ts.x, ts.y, /*allowFlip=*/false);
    std::vector<uint8_t> buffer(ts.x * ts.y);
    m_texture.update(buffer.data(), {0, 0, ts.x, ts.y});
}


} // namespace xci::text
