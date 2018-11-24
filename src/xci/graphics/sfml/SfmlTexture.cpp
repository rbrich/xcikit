// SfmlTexture.cpp created on 2018-03-04, part of XCI toolkit
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

#include "SfmlTexture.h"

namespace xci::graphics {


bool SfmlTexture::create(const Vec2u& size)
{
    unsigned width = std::min(size.x, sf::Texture::getMaximumSize());
    unsigned height = std::min(size.y, sf::Texture::getMaximumSize());

    return m_texture.create(width, height);
}


void SfmlTexture::update(const uint8_t* pixels)
{
    // transform the bitmap into 32bit RGBA format
    auto size = m_texture.getSize();
    std::vector<uint8_t> buffer(size.x * size.y * 4, 0xFF);
    for (unsigned i = 0; i < size.x * size.y; i++) {
        buffer[4*i+3] = pixels[i];
    }

    m_texture.update(buffer.data());
}


void SfmlTexture::update(const uint8_t* pixels, const Rect_u& region)
{
    // transform the bitmap into 32bit RGBA format
    std::vector<uint8_t> buffer(region.w * region.h * 4, 0xFF);
    for (unsigned i = 0; i < region.w * region.h; i++) {
        buffer[4*i+3] = pixels[i];
    }

    m_texture.update(buffer.data(), region.w, region.h, region.x, region.y);
}


Vec2u SfmlTexture::size() const
{
    return {m_texture.getSize().x, m_texture.getSize().y};
}


} // namespace xci::graphics
