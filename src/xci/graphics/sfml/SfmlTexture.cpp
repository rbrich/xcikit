// SfmlTexture.cpp created on 2018-03-04, part of XCI toolkit

#include "SfmlTexture.h"

// inline
#include <xci/graphics/Texture.inl>

namespace xci {
namespace graphics {


bool SfmlTexture::create(unsigned int width, unsigned int height)
{
    width = std::min(width, sf::Texture::getMaximumSize());
    height = std::min(height, sf::Texture::getMaximumSize());

    return m_texture.create(width, height);
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


}} // namespace xci::graphics
