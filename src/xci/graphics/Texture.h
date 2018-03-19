// Texture.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_TEXTURE_H
#define XCI_GRAPHICS_TEXTURE_H

#include <xci/util/geometry.h>

#include <memory>
#include <cstdint>

namespace xci {
namespace graphics {

using xci::util::Vec2u;
using xci::util::Rect_u;
using std::uint8_t;


class Texture {
public:
    Texture();
    ~Texture();
    Texture(Texture&&) noexcept;
    Texture& operator=(Texture&&) noexcept;

    bool create(unsigned int width, unsigned int height);
    void update(const uint8_t* pixels, const Rect_u& region);

    Vec2u size() const;

    class Impl;
    const Impl& impl() const { return *m_impl; }

private:
    std::unique_ptr<Impl> m_impl;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_TEXTURE_H
