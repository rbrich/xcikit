// SpritesImpl.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_SFML_SPRITESIMPL_H
#define XCI_GRAPHICS_SFML_SPRITESIMPL_H

#include "TextureImpl.h"

#include <xci/graphics/Sprites.h>

namespace xci {
namespace graphics {

class SpritesImpl  {
public:
    explicit SpritesImpl(const Texture& texture);

    PT(GeomVertexData) vertex_data;
    PT(GeomTrifans) trifans;
    PT(::Texture) texture;
};

}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SFML_SPRITESIMPL_H
