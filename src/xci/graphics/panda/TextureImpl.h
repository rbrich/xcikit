// TextureImpl.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_SFML_TEXTUREIMPL_H
#define XCI_GRAPHICS_SFML_TEXTUREIMPL_H

#include <xci/graphics/Texture.h>

#include <texture.h>
#include <pointerTo.h>

namespace xci {
namespace graphics {

class TextureImpl {
public:
    PT(::Texture) texture;
};

}} // namespace xci::graphics

#endif //XCI_GRAPHICS_SFML_TEXTUREIMPL_H
