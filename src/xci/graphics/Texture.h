// Texture.h created on 2018-03-04, part of XCI toolkit
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
    virtual ~Texture() = default;

    virtual bool create(const Vec2u& size) = 0;
    virtual void update(const uint8_t* pixels) = 0;
    virtual void update(const uint8_t* pixels, const Rect_u& region) = 0;

    virtual Vec2u size() const = 0;
};


using TexturePtr = std::shared_ptr<Texture>;


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_TEXTURE_H
