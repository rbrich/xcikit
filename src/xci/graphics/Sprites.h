// Sprites.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_SPRITES_H
#define XCI_GRAPHICS_SPRITES_H

#include <xci/graphics/Texture.h>
#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>
#include <xci/util/geometry.h>

#include <memory>

namespace xci {
namespace graphics {

using xci::util::Rect_u;
using xci::util::Rect_f;
using xci::util::Vec2f;

// A collection of sprites (ie. alpha-blended textured quads)
// sharing the same texture. Each sprite can display different
// part of the texture.

class Sprites {
public:
    explicit Sprites(const Texture& texture);
    ~Sprites();
    Sprites(Sprites&&) noexcept;
    Sprites& operator=(Sprites&&) noexcept;

    // Add new sprite containing whole texture
    // `rect` defines position and size of the sprite
    void add_sprite(const Rect_f& rect, const Color& color);

    // Add new sprite containing a cutoff from the texture
    // `rect` defines position and size of the sprite
    void add_sprite(const Rect_f& rect, const Rect_u& texrect,
                    const Color& color);

    // Draw all sprites to `view` at `pos`.
    // Final sprite position is `pos` + sprite's relative position
    void draw(View& view, const Vec2f& pos);

    class Impl;
    const Impl& impl() const { return *m_impl; }

private:
    std::unique_ptr<Impl> m_impl;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SPRITES_H
