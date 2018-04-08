// GlView.h created on 2018-03-14, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_GL_VIEW_H
#define XCI_GRAPHICS_GL_VIEW_H

#include <xci/graphics/View.h>

namespace xci {
namespace graphics {


class GlView {
public:
    void set_screen_size(Vec2u size);
    Vec2u screen_size() const { return m_screen_size; }

    void set_framebuffer_size(Vec2u size);
    Vec2u framebuffer_size() const { return m_framebuffer_size; }

    Vec2f scalable_size() const { return m_scalable_size; }

private:
    Vec2f m_scalable_size;      // eg. {2.666, 2.0}
    Vec2u m_screen_size;        // eg. {800, 600}
    Vec2u m_framebuffer_size;   // eg. {1600, 1200}
};


class View::Impl : public GlView {};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_GL_VIEW_H
