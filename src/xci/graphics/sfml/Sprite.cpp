// Sprite.cpp created on 2018-03-04, part of XCI toolkit

#include "SpriteImpl.h"

namespace xci {
namespace graphics {


Sprite::Sprite(const Texture& texture)
        : m_impl(new SpriteImpl(texture.impl())) {}

Sprite::Sprite(const Texture& texture, const Rect_u& rect)
        : m_impl(new SpriteImpl(texture.impl(),
                                {(int)rect.x, (int)rect.y,
                                 (int)rect.w, (int)rect.h})) {}

Sprite::~Sprite()
{
    delete m_impl;
}


}} // namespace xci::graphics
