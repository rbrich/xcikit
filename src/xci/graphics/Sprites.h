// Sprites.h created on 2018-03-04, part of XCI toolkit
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef XCI_GRAPHICS_SPRITES_H
#define XCI_GRAPHICS_SPRITES_H

#include <xci/graphics/Renderer.h>
#include <xci/graphics/Primitives.h>
#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>
#include <xci/util/geometry.h>

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
    explicit Sprites(TexturePtr& texture,
                     const Color& color = Color::White(),
                     Renderer& renderer = Renderer::default_renderer());

    // Add new sprite containing whole texture
    // `rect` defines position and size of the sprite
    void add_sprite(const Rect_f& rect);

    // Add new sprite containing a cutoff from the texture
    // `rect` defines position and size of the sprite
    void add_sprite(const Rect_f& rect, const Rect_u& texrect);

    // Draw all sprites to `view` at `pos`.
    // Final sprite position is `pos` + sprite's relative position
    void draw(View& view, const Vec2f& pos);

private:
    void init_shader();

private:
    TexturePtr m_texture;
    Color m_color;
    PrimitivesPtr m_trifans;
    ShaderPtr m_shader;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SPRITES_H
