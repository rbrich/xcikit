// SfmlSprites.cpp created on 2018-03-04, part of XCI toolkit

#include "SfmlSprites.h"
#include "SfmlTexture.h"
#include "SfmlView.h"

// inline
#include <xci/graphics/Sprites.inl>

namespace xci {
namespace graphics {


SfmlSprites::SfmlSprites(const Texture& texture)
        : m_texture(texture.impl().sfml_texture()) {}


void SfmlSprites::add_sprite(const Vec2f& pos, const Color& color)
{
    sf::Sprite sprite(m_texture);
    sprite.setPosition(pos.x, pos.y);
    sprite.setColor(sf::Color(color.r, color.g, color.b, color.a));
    m_sprites.push_back(sprite);
}


// Position a sprite with cutoff from the texture
void SfmlSprites::add_sprite(const Vec2f& pos, const Rect_u& texrect, const Color& color)
{
    sf::Sprite sprite(m_texture, {(int)texrect.x, (int)texrect.y,
                                  (int)texrect.w, (int)texrect.h});
    sprite.setPosition(pos.x, pos.y);
    sprite.setColor(sf::Color(color.r, color.g, color.b, color.a));
    m_sprites.push_back(sprite);
}


void SfmlSprites::draw(View& view, const Vec2f& pos)
{
    sf::RenderStates states;
    states.transform.translate(pos.x, pos.y);
    states.blendMode = sf::BlendAlpha;

    auto& target = view.impl().sfml_target();
    for (auto& sprite : m_sprites) {
        target.draw(sprite, states);
    }
}


}} // namespace xci::graphics
