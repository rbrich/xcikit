// SpriteImpl.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_SFML_SPRITEIMPL_H
#define XCI_GRAPHICS_SFML_SPRITEIMPL_H

#include "TextureImpl.h"

#include <xci/graphics/Sprite.h>

#include <SFML/Graphics/Sprite.hpp>

namespace xci {
namespace graphics {

class SpriteImpl : public sf::Sprite {
public:
    explicit SpriteImpl(const TextureImpl& texture) : sf::Sprite(texture) {}
    SpriteImpl(const TextureImpl& texture, const sf::IntRect& rectangle)
            : sf::Sprite(texture, rectangle) {}
};

}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SFML_SPRITEIMPL_H
