// GlTexture.h created on 2018-03-14, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_GL_TEXTURE_H
#define XCI_GRAPHICS_GL_TEXTURE_H

#include <xci/graphics/Texture.h>

#include <GL/glew.h>

namespace xci {
namespace graphics {


class GlTexture {
public:
    bool create(unsigned int width, unsigned int height);
    void update(const uint8_t* pixels, const Rect_u& region);

    unsigned int width() const;
    unsigned int height() const;

    // access native handle
    GLuint gl_texture() const { return m_texture; }

private:
    GLuint m_texture = 0;
    Vec2u m_size;
    bool m_smooth = false;
};


class Texture::Impl : public GlTexture {};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_GL_TEXTURE_H
