// PandaTexture.cpp created on 2018-03-04, part of XCI toolkit

#include "PandaTexture.h"

// inline
#include <xci/graphics/Texture.inl>

namespace xci {
namespace graphics {


bool PandaTexture::create(unsigned int width, unsigned int height)
{
    m_texture = new ::Texture();
    m_texture->setup_2d_texture(int(width), int(height),
                                ::Texture::T_unsigned_byte,
                                ::Texture::F_alpha);
    m_texture->set_keep_ram_image(true);
    return true;
}

void PandaTexture::update(const uint8_t* pixels, const Rect_u& region)
{
    auto img = m_texture->modify_ram_image();
    for (unsigned int y = region.y; y < region.y + region.h; y++) {
        uint8_t* row = &img[size_t(y * m_texture->get_x_size()) + region.x];
        for (unsigned int x = region.x; x < region.x + region.w; x++) {
            *row++ = *pixels++;
        }
    }
}

Vec2u PandaTexture::size() const
{
    return {(unsigned int) m_texture->get_x_size(),
            (unsigned int) m_texture->get_y_size()};
}


}} // namespace xci::graphics
