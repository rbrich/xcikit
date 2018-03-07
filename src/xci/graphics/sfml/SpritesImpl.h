// SpriteImpl.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_SFML_SPRITEIMPL_H
#define XCI_GRAPHICS_SFML_SPRITEIMPL_H

#include "TextureImpl.h"

#include <xci/graphics/Sprites.h>

#include <SFML/Graphics/Sprite.hpp>

namespace xci {
namespace graphics {

class SpritesImpl {
public:
    explicit SpritesImpl(const Texture& texture) : texture(texture.impl()) {}

    std::vector<sf::Sprite> sprites;
    sf::Texture texture;
};

}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SFML_SPRITEIMPL_H
