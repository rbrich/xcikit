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

namespace xci::graphics {


bool GlTexture::create(const Vec2u& size)
{
    destroy();
    m_size = size;
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    // Just allocate the memory. Content is left undefined.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_size.x, m_size.y, 0,
                 GL_RED, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return true;
}


void GlTexture::update(const uint8_t* pixels)
{
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_size.x, m_size.y, 0,
                 GL_RED, GL_UNSIGNED_BYTE, pixels);
}


void GlTexture::update(const uint8_t* pixels, const Rect_u& region)
{
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, region.x, region.y, region.w, region.h,
                    GL_RED, GL_UNSIGNED_BYTE, pixels);
}


Vec2u GlTexture::size() const { return m_size; }


void GlTexture::destroy()
{
    if (m_texture == 0)
        return;

    glDeleteTextures(1, &m_texture);
}


} // namespace xci::graphics
