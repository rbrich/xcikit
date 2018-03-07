// Sprite.cpp created on 2018-03-04, part of XCI toolkit

#include "SpritesImpl.h"
#include "TextureImpl.h"

namespace xci {
namespace graphics {


Sprites::Sprites(const Texture& texture) : m_impl(new SpritesImpl(texture)) {}
Sprites::~Sprites() { delete m_impl; }

void Sprites::add_sprite(const Vec2f& pos, const Color& color)
{
    m_impl->sprites.emplace_back(m_impl->texture);
}

// Position a sprite with cutoff from the texture
void Sprites::add_sprite(const Vec2f& pos, const Rect_u& texrect, const Color& color)
{
    m_impl->sprites.emplace_back(m_impl->texture,
                                 {(int)texrect.x, (int)texrect.y,
                                  (int)texrect.w, (int)texrect.h});
}

void Sprites::draw(View& view, const Vec2f& pos);


}} // namespace xci::graphics
