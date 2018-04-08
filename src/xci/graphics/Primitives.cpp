// Primitives.cpp created on 2018-04-08, part of XCI toolkit
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

#include "Primitives.h"

#ifdef XCI_WITH_OPENGL
#include <xci/graphics/opengl/GlPrimitives.h>
#endif

namespace xci {
namespace graphics {


// Forward to Primitives::Impl
Primitives::Primitives(VertexFormat format) : m_impl(new Impl(format)) {}
Primitives::~Primitives() = default;
Primitives::Primitives(Primitives&&) noexcept = default;
Primitives& Primitives::operator=(Primitives&&) noexcept = default;

void Primitives::begin_primitive() { m_impl->begin_primitive(); }
void Primitives::end_primitive() { m_impl->end_primitive(); }
void Primitives::add_vertex(float x, float y, float u, float v) { m_impl->add_vertex(x, y, u, v); }
void Primitives::add_vertex(float x, float y, float u1, float v1, float u2, float v2) { m_impl->add_vertex(x, y, u1, v1, u2, v2); }
void Primitives::clear() { m_impl->clear(); }
bool Primitives::empty() const { return m_impl->empty(); }

void Primitives::set_shader(ShaderPtr& shader) { m_impl->set_shader(shader); }
void Primitives::set_uniform(const char* name, float f) { m_impl->set_uniform(name, f); }
void Primitives::set_uniform(const char* name, float f1, float f2, float f3, float f4) { m_impl->set_uniform(name, f1, f2, f3, f4); }
void Primitives::set_texture(const char* name, TexturePtr& texture) { m_impl->set_texture(name, texture); }
void Primitives::draw(View& view, const Vec2f& pos) { m_impl->draw(view, pos); }


}} // namespace xci::graphics
