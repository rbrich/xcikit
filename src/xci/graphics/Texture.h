// Texture.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_TEXTURE_H
#define XCI_GRAPHICS_TEXTURE_H

#include <xci/util/geometry.h>
using xci::util::Vec2u;
using xci::util::Rect_u;

#include <memory>
#include <cstdint>
using std::uint8_t;

namespace xci {
namespace graphics {


class Texture {
public:
    Texture();
    ~Texture();
    Texture(Texture&&);
    Texture& operator=(Texture&&);

    bool create(unsigned int width, unsigned int height);
    void update(const uint8_t* pixels, const Rect_u& region);

    unsigned int width() const;
    unsigned int height() const;

    class Impl;
    const Impl& impl() const { return *m_impl; }

private:
    std::unique_ptr<Impl> m_impl;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_TEXTURE_H
