// Sprites.h created on 2018-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_SPRITES_H
#define XCI_GRAPHICS_SPRITES_H

#include <xci/graphics/Renderer.h>
#include <xci/graphics/Primitives.h>
#include <xci/graphics/Color.h>
#include <xci/core/geometry.h>

namespace xci::graphics { class View; }

namespace xci::graphics {

using xci::core::Rect_u;
using xci::core::Rect_f;
using xci::core::Vec2f;

// A collection of sprites (ie. alpha-blended textured quads)
// sharing the same texture. Each sprite can display different
// part of the texture.

class Sprites {
public:
    explicit Sprites(Renderer& renderer, Texture& texture,
                     const Color& color = Color::White());

    // Reserve memory for `num` sprites.
    void reserve(size_t num);

    // Clear all sprites.
    void clear() { m_quads.clear(); }

    // Add new sprite containing whole texture
    // `rect` defines position and size of the sprite
    void add_sprite(const ViewportRect& rect);

    // Add new sprite containing a cutoff from the texture
    // `rect` defines position and size of the sprite
    void add_sprite(const ViewportRect& rect, const Rect_u& texrect);

    // Update sprites attributes according to settings (color etc.)
    void update();

    // Draw all sprites to `view` at `pos`.
    // Final sprite position is `pos` + sprite's relative position
    void draw(View& view, const ViewportCoords& pos);

private:
    Texture& m_texture;
    Color m_color;
    Primitives m_quads;
    Shader& m_shader;
};


// Similar to Sprites, but allows using a different color for each sprite.

class ColoredSprites {
public:
    explicit ColoredSprites(Renderer& renderer, Texture& texture,
                            const Color& color = Color::White());

    // Reserve memory for `num` sprites.
    void reserve(size_t num);
    void clear() { m_quads.clear(); }

    void set_color(const Color& color) { m_color = color; }
    const Color& color() const { return m_color; }

    // Add new sprite containing whole texture
    // `rect` defines position and size of the sprite
    void add_sprite(const ViewportRect& rect);

    // Add new sprite containing a cutoff from the texture
    // `rect` defines position and size of the sprite
    void add_sprite(const ViewportRect& rect, const Rect_u& texrect);

    // Update sprites attributes according to settings (color etc.)
    void update();

    // Draw all sprites to `view` at `pos`.
    // Final sprite position is `pos` + sprite's relative position
    void draw(View& view, const ViewportCoords& pos);

private:
    Texture& m_texture;
    Color m_color;
    Primitives m_quads;
    Shader& m_shader;
};


} // namespace xci::graphics

#endif // XCI_GRAPHICS_SPRITES_H
