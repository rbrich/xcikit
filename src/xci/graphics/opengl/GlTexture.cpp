// GlTexture.cpp created on 2018-03-14, part of XCI toolkit
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

#include "GlTexture.h"

#include <vector>

// inline
#include <xci/graphics/Texture.inl>

namespace xci {
namespace graphics {


bool GlTexture::create(unsigned int width, unsigned int height)
{
    m_size = {width, height};
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    std::vector<uint8_t> buffer(width * height, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0,
                 GL_RED, GL_UNSIGNED_BYTE, buffer.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_smooth ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_smooth ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return true;
}

void GlTexture::update(const uint8_t* pixels, const Rect_u& region)
{
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, region.x, region.y, region.w, region.h,
                    GL_RED, GL_UNSIGNED_BYTE, pixels);
}

unsigned int GlTexture::width() const { return m_size.x; }
unsigned int GlTexture::height() const { return m_size.y; }


}} // namespace xci::graphics
