// Texture.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_SFML_TEXTURE_H
#define XCI_GRAPHICS_SFML_TEXTURE_H

#include <xci/util/geometry.h>
using xci::util::Vec2u;
using xci::util::Rect_u;

#include <SFML/Graphics/Texture.hpp>

namespace xci {
namespace graphics {

class Texture
{
public:
    bool create(unsigned int width, unsigned int height) {
        return sfml_texture.create(width, height);
    }

    void update(const uint8_t* pixels, const Rect_u& region) {
        sfml_texture.update(pixels, region.w, region.h, region.x, region.y);
    }

    Vec2u size() const { return Vec2u(sfml_texture.getSize()); }
    unsigned int width() const { return sfml_texture.getSize().x; }
    unsigned int height() const { return sfml_texture.getSize().y; }

    const sf::Texture& sfml() const { return sfml_texture; }

    static unsigned int maximum_size() { return sf::Texture::getMaximumSize(); }

private:
    sf::Texture sfml_texture;
};

}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SFML_TEXTURE_H
