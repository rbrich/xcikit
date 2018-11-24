// SfmlTexture.h created on 2018-03-04, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_SFML_TEXTURE_H
#define XCI_GRAPHICS_SFML_TEXTURE_H

#include <xci/graphics/Texture.h>

#include <SFML/Graphics/Texture.hpp>

namespace xci::graphics {


class SfmlTexture : public Texture {
public:
    bool create(const Vec2u& size) override;
    void update(const uint8_t* pixels) override;
    void update(const uint8_t* pixels, const Rect_u& region) override;

    Vec2u size() const override;

    // access native object
    const sf::Texture& sfml_texture() const { return m_texture; }

private:
    sf::Texture m_texture;
};


} // namespace xci::graphics

#endif //XCI_GRAPHICS_SFML_TEXTURE_H
