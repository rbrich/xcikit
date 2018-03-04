// Sprite.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_SFML_SPRITE_H
#define XCI_GRAPHICS_SFML_SPRITE_H

#include <xci/graphics/Texture.h>

#include <xci/util/geometry.h>
using xci::util::Vec2f;
using xci::util::Rect_u;

#include <SFML/Graphics/Sprite.hpp>

namespace xci {
namespace graphics {

class Sprite {
public:
    explicit Sprite(const Texture& texture) : sfml_sprite(texture.sfml()) {}
    explicit Sprite(const Texture& texture, const Rect_u& rect)
        : sfml_sprite(texture.sfml(), {(int)rect.x, (int)rect.y,
                                       (int)rect.w, (int)rect.h}) {}

    //void set_position(const Vec2f& pos) { sfml_sprite.setPosition(pos.x, pos.y); }

    const sf::Sprite& sfml() const { return sfml_sprite; }

private:
    sf::Sprite sfml_sprite;
};

}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SFML_SPRITE_H
