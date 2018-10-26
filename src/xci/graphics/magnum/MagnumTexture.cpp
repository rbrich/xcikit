// MagnumTexture.cpp created on 2018-10-26, part of XCI toolkit
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

#include "MagnumTexture.h"

#include <Magnum/GL/TextureFormat.h>
#include <Magnum/ImageView.h>

namespace xci::graphics {

using namespace Magnum;


bool MagnumTexture::create(const xci::core::Vec2u& size)
{
    m_size = size;
    m_texture.setWrapping(GL::SamplerWrapping::ClampToEdge)
             .setMagnificationFilter(GL::SamplerFilter::Nearest)
             .setMinificationFilter(GL::SamplerFilter::Nearest)
             .setStorage(1, GL::TextureFormat::Red, {int(size.x), int(size.y)});
    return true;
}


void MagnumTexture::update(const uint8_t* pixels)
{
    ImageView2D image{PixelStorage{}.setAlignment(1), PixelFormat::R8UI,
                      {int(m_size.x), int(m_size.y)},
                      {pixels, size_t(m_size.x * m_size.y)}};
    m_texture.setSubImage(0, {}, image);
}


void MagnumTexture::update(const uint8_t* pixels, const xci::core::Rect_u& region)
{
    ImageView2D image{PixelStorage{}.setAlignment(1), PixelFormat::R8UI,
                      {int(region.w), int(region.h)},
                      {pixels, size_t(region.w * region.h)}};
    m_texture.setSubImage(0, {int(region.x), int(region.y)}, image);
}


xci::core::Vec2u MagnumTexture::size() const { return m_size; }


} // namespace xci::graphics
