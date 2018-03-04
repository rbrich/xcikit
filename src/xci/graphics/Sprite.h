// Sprite.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_SPRITE_H
#define XCI_GRAPHICS_SPRITE_H

#include <xci/graphics/Texture.h>

#include <xci/util/geometry.h>
using xci::util::Rect_u;

namespace xci {
namespace graphics {


class SpriteImpl;

class Sprite {
public:
    explicit Sprite(const Texture& texture);
    explicit Sprite(const Texture& texture, const Rect_u& rect);
    ~Sprite();

    const SpriteImpl& impl() const { return *m_impl; }

private:
    SpriteImpl* m_impl;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SPRITE_H
