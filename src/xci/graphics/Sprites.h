// Sprites.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_SPRITES_H
#define XCI_GRAPHICS_SPRITES_H

#include <xci/graphics/Texture.h>
#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>
#include <xci/util/geometry.h>

using xci::util::Rect_u;
using xci::util::Vec2f;

namespace xci {
namespace graphics {


class SpritesImpl;

class Sprites {
public:
    explicit Sprites(const Texture& texture);
    ~Sprites();

    // Position a sprite with whole texture
    void add_sprite(const Vec2f& pos, const Color& color);

    // Position a sprite with cutoff from the texture
    void add_sprite(const Vec2f& pos, const Rect_u& texrect,
                    const Color& color);

    void draw(View& view, const Vec2f& pos);

    const SpritesImpl& impl() const { return *m_impl; }

private:
    SpritesImpl* m_impl;
};

}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SPRITES_H
