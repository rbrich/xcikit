// Texture.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_TEXTURE_H
#define XCI_GRAPHICS_TEXTURE_H

#include <xci/util/geometry.h>
using xci::util::Vec2u;
using xci::util::Rect_u;

#include <cstdint>
using std::uint8_t;

namespace xci {
namespace graphics {


class TextureImpl;

class Texture {
public:
    Texture();
    ~Texture();

    static unsigned int maximum_size();

    bool create(unsigned int width, unsigned int height);
    void update(const uint8_t* pixels, const Rect_u& region);

    Vec2u size() const;
    unsigned int width() const;
    unsigned int height() const;

    const TextureImpl& impl() const { return *m_impl; }

private:
    TextureImpl* m_impl;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_TEXTURE_H
