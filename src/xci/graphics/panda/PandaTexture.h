// PandaTexture.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_PANDA_TEXTURE_H
#define XCI_GRAPHICS_PANDA_TEXTURE_H

#include <xci/graphics/Texture.h>

#include <texture.h>
#include <pointerTo.h>

namespace xci {
namespace graphics {

class PandaTexture {
public:
    bool create(unsigned int width, unsigned int height);
    void update(const uint8_t* pixels, const Rect_u& region);

    unsigned int width() const;
    unsigned int height() const;

    // access native handle
    PT(::Texture) panda_texture() const { return m_texture; }

private:
    PT(::Texture) m_texture;
};

class Texture::Impl : public PandaTexture {};

}} // namespace xci::graphics

#endif //XCI_GRAPHICS_PANDA_TEXTURE_H
