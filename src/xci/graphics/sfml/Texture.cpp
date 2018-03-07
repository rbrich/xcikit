// Texture.cpp created on 2018-03-04, part of XCI toolkit

#include "TextureImpl.h"

namespace xci {
namespace graphics {


Texture::Texture() : m_impl(new TextureImpl) {}
Texture::~Texture() { delete m_impl; }


bool Texture::create(unsigned int width, unsigned int height)
{
    unsigned int width = std::min(width, sf::Texture::getMaximumSize());
    unsigned int height = std::min(height, sf::Texture::getMaximumSize());

    return m_impl->create(width, height);
}

void Texture::update(const uint8_t* pixels, const Rect_u& region)
{
    // transform the bitmap into 32bit RGBA format
    std::vector<uint8_t> buffer(region.w * region.h * 4, 0xFF);
    for (unsigned i = 0; i < region.w * region.h; i++) {
        buffer[4*i+3] = pixels[i];
    }

    m_impl->update(buffer.data(), region.w, region.h, region.x, region.y);
}

unsigned int Texture::width() const { return m_impl->getSize().x; }
unsigned int Texture::height() const { return m_impl->getSize().y; }


}} // namespace xci::graphics
