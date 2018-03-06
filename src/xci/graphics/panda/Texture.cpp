// Texture.cpp created on 2018-03-04, part of XCI toolkit

#include "TextureImpl.h"

namespace xci {
namespace graphics {


Texture::Texture() : m_impl(new TextureImpl) {}
Texture::~Texture() { delete m_impl; }

unsigned int Texture::maximum_size()
{
    // TODO: leave this only inside SFML implementation
    return 2048;
}

bool Texture::create(unsigned int width, unsigned int height)
{
    m_impl->texture = new ::Texture();
    ::Texture* texture = m_impl->texture;

    texture->setup_2d_texture(int(width), int(height),
                              ::Texture::T_unsigned_byte,
                              ::Texture::F_rgba);
    texture->set_keep_ram_image(true);
    return true;
}

void Texture::update(const uint8_t* pixels, const Rect_u& region)
{
    ::Texture* texture = m_impl->texture;
    auto img = texture->modify_ram_image();
    for (int y = region.y; y < region.y + region.h; y++) {
        uint8_t* row = &img[size_t(y * texture->get_x_size()) + 4 * region.x];
        for (int x = region.x; x < region.x + region.w; x++) {
            *row++ = *pixels++;
            *row++ = *pixels++;
            *row++ = *pixels++;
            *row++ = *pixels++;
        }
    }
}

unsigned int Texture::width() const
{
    return (unsigned) m_impl->texture->get_x_size();
}

unsigned int Texture::height() const
{
    return (unsigned) m_impl->texture->get_y_size();
}


}} // namespace xci::graphics
