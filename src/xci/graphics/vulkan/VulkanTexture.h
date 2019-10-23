// VulkanTexture.h created on 2019-10-23, part of XCI toolkit
// Copyright 2019 Radek Brich
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

#ifndef XCI_GRAPHICS_VULKAN_TEXTURE_H
#define XCI_GRAPHICS_VULKAN_TEXTURE_H

#include <xci/graphics/Texture.h>

namespace xci::graphics {


class VulkanTexture : public Texture {
public:
    bool create(const Vec2u& size) override;

    void update(const uint8_t* pixels) override;
    void update(const uint8_t* pixels, const Rect_u& region) override;

    Vec2u size() const override;

private:

};


} // namespace xci::graphics

#endif // include guard
