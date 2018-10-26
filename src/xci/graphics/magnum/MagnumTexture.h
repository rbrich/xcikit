// MagnumTexture.h created on 2018-10-26, part of XCI toolkit
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

#ifndef XCIKIT_MAGNUMTEXTURE_H
#define XCIKIT_MAGNUMTEXTURE_H

#include <xci/graphics/Texture.h>

#include <Magnum/GL/Texture.h>

namespace xci::graphics {


class MagnumTexture : public Texture {
public:
    bool create(const Vec2u& size) override;
    void update(const uint8_t* pixels) override;
    void update(const uint8_t* pixels, const Rect_u& region) override;

    Vec2u size() const override;

private:
    Magnum::GL::Texture2D m_texture;
    Vec2u m_size;
};


} // namespace xci::graphics

#endif // include guard
