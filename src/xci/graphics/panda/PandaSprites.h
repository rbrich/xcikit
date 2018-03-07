// SpritesImpl.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_PANDA_SPRITES_H
#define XCI_GRAPHICS_PANDA_SPRITES_H

#include "PandaTexture.h"

#include <xci/graphics/Sprites.h>

namespace xci {
namespace graphics {


class PandaSprites {
public:
    explicit PandaSprites(const Texture& texture);

    void add_sprite(const Vec2f& pos, const Color& color);
    void add_sprite(const Vec2f& pos, const Rect_u& texrect, const Color& color);
    void draw(View& view, const Vec2f& pos);

private:
    PT(GeomVertexData) m_vertex_data;
    PT(GeomTrifans) m_trifans;
    PT(::Texture) m_texture;
};


class Sprites::Impl : public PandaSprites {
public:
    explicit Impl(const Texture& texture) : PandaSprites(texture) {}
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_PANDA_SPRITES_H
