// Texture.cpp created on 2018-03-04, part of XCI toolkit

#include "TextureImpl.h"

namespace xci {
namespace graphics {


Texture::Texture() : m_impl(new TextureImpl) {}
Texture::~Texture() { delete m_impl; }

unsigned int Texture::maximum_size()
{
    return sf::Texture::getMaximumSize();
}

bool Texture::create(unsigned int width, unsigned int height)
{
    return m_impl->create(width, height);
}

void Texture::update(const uint8_t* pixels, const Rect_u& region)
{
    m_impl->update(pixels, region.w, region.h, region.x, region.y);
}

Vec2u Texture::size() const
{
    return Vec2u(m_impl->getSize());
}

unsigned int Texture::width() const { return m_impl->getSize().x; }
unsigned int Texture::height() const { return m_impl->getSize().y; }


}} // namespace xci::graphics
