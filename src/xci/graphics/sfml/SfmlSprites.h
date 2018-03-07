// SfmlSprites.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_SFML_SPRITES_H
#define XCI_GRAPHICS_SFML_SPRITES_H

#include "SfmlTexture.h"

#include <xci/graphics/Sprites.h>

#include <SFML/Graphics/Sprite.hpp>

namespace xci {
namespace graphics {


class SfmlSprites {
public:
    explicit SfmlSprites(const Texture& texture);

    void add_sprite(const Vec2f& pos, const Color& color);
    void add_sprite(const Vec2f& pos, const Rect_u& texrect, const Color& color);
    void draw(View& view, const Vec2f& pos);

private:
    std::vector<sf::Sprite> m_sprites;
    sf::Texture m_texture;
};


class Sprites::Impl : public SfmlSprites {
public:
    explicit Impl(const Texture& texture) : SfmlSprites(texture) {}
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SFML_SPRITES_H
