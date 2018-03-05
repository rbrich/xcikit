// Texture.cpp created on 2018-03-04, part of XCI toolkit

#include "TextureImpl.h"

namespace xci {
namespace graphics {


Texture::Texture() : m_impl(new TextureImpl) {}
Texture::~Texture() { delete m_impl; }

unsigned int Texture::maximum_size()
{
    return 512;
}

bool Texture::create(unsigned int width, unsigned int height)
{
    return true;
}

void Texture::update(const uint8_t* pixels, const Rect_u& region)
{

}

Vec2u Texture::size() const
{
    return {512, 512};
}

unsigned int Texture::width() const { return 512; }
unsigned int Texture::height() const { return 512; }


}} // namespace xci::graphics
