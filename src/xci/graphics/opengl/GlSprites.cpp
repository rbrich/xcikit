// GlSprites.cpp created on 2018-03-14, part of XCI toolkit
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

#include "GlSprites.h"
#include "GlTexture.h"
#include "GlView.h"

#include <xci/util/log.h>

// inline
#include <xci/graphics/Sprites.inl>

#ifdef XCI_EMBED_SHADERS
#define INCBIN_PREFIX g_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "incbin.h"
INCBIN(sprite_vert, XCI_SHARE_DIR "/shaders/sprite.vert");
INCBIN(sprite_frag, XCI_SHARE_DIR "/shaders/sprite.frag");
#else
static const unsigned char* g_sprite_vert_data = nullptr;
static const unsigned int g_sprite_vert_size = 0;
static const unsigned char* g_sprite_frag_data = nullptr;
static const unsigned int g_sprite_frag_size = 0;
#endif


namespace xci {
namespace graphics {

using namespace xci::util::log;


GlSprites::GlSprites(const Texture& texture, const Color& color)
        : m_texture(texture), m_color(color) {}


void GlSprites::add_sprite(const Rect_f& rect)
{
    auto ts = m_texture.size();
    add_sprite(rect, {0, 0, ts.x, ts.y});
}


// Position a sprite with cutoff from the texture
void GlSprites::add_sprite(const Rect_f& rect, const Rect_u& texrect)
{
    clear_gl_objects();

    float x1 = rect.x;
    float y1 = -rect.y;
    float x2 = rect.x + rect.w;
    float y2 = -rect.y - rect.h;
    auto ts = m_texture.size();
    float tl = (float)texrect.left() / ts.x;
    float tr = (float)texrect.right() / ts.x;
    float tb = (float)texrect.bottom() / ts.y;
    float tt = (float)texrect.top() / ts.y;
    GLushort i = (GLushort) m_vertex_data.size();
    m_vertex_data.push_back({x2, y1, tr, tt});
    m_vertex_data.push_back({x2, y2, tr, tb});
    m_vertex_data.push_back({x1, y2, tl, tb});
    m_vertex_data.push_back({x1, y1, tl, tt});
    m_elem_first.push_back(i);
    m_elem_size.push_back(4);
}


void GlSprites::draw(View& view, const Vec2f& pos)
{
    init_gl_objects();

    auto program = view.impl()
            .gl_program(GlView::ProgramId::Sprite,
                        "shaders/sprite.vert", "shaders/sprite.frag",
                        (const char*)g_sprite_vert_data, g_sprite_vert_size,
                        (const char*)g_sprite_frag_data, g_sprite_frag_size);
    glUseProgram(program);
    glBindVertexArray(m_vertex_array);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // projection matrix
    GLfloat xs = 2.0f / view.scalable_size().x;
    GLfloat ys = 2.0f / view.scalable_size().y;
    GLfloat xt = pos.x * xs;
    GLfloat yt = pos.y * ys;
    const GLfloat mvp[] = {
            xs,   0.0f, 0.0f, 0.0f,
            0.0f, ys,   0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            xt,  -yt,   0.0f, 1.0f,
    };

    GLint u_mvp = glGetUniformLocation(program, "u_mvp");
    glUniformMatrix4fv(u_mvp, 1, GL_FALSE, (const GLfloat*) mvp);

    GLint u_color = glGetUniformLocation(program, "u_color");
    glUniform4f(u_color, m_color.red_f(), m_color.green_f(),
                m_color.blue_f(), m_color.alpha_f());

    GLint tex_location = glGetUniformLocation(program, "u_texture");
    glUniform1i(tex_location, 0); // GL_TEXTURE0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture.impl().gl_texture());
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMultiDrawArrays(GL_TRIANGLE_FAN, m_elem_first.data(), m_elem_size.data(),
                      (GLsizei) m_elem_size.size());

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glUseProgram(0);
}


void GlSprites::init_gl_objects()
{
    if (m_objects_ready)
        return;

    glGenVertexArrays(1, &m_vertex_array);
    glBindVertexArray(m_vertex_array);

    glGenBuffers(1, &m_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_vertex_data.size(),
                 m_vertex_data.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) (sizeof(GLfloat) * 0));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) (sizeof(GLfloat) * 2));

    m_objects_ready = true;
}


void GlSprites::clear_gl_objects()
{
    if (!m_objects_ready)
        return;

    glDeleteVertexArrays(1, &m_vertex_array);
    glDeleteBuffers(1, &m_vertex_buffer);

    m_objects_ready = false;
}


}} // namespace xci::graphics
