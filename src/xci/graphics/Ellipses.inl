// Ellipses.inl created on 2018-03-24, part of XCI toolkit
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

namespace xci {
namespace graphics {


// Forward to Ellipses::Impl
Ellipses::Ellipses(const Color& fill_color, const Color& outline_color)
        : m_impl(new Impl(fill_color, outline_color)) {}
Ellipses::~Ellipses() = default;
Ellipses::Ellipses(Ellipses&&) noexcept = default;
Ellipses& Ellipses::operator=(Ellipses&&) noexcept = default;

void Ellipses::add_ellipse(const Rect_f& rect, float outline_thickness)
{ m_impl->add_ellipse(rect, outline_thickness); }

void Ellipses::clear_ellipses() { m_impl->clear_ellipses(); }

void Ellipses::draw(View& view, const Vec2f& pos)
{ m_impl->draw(view, pos); }


}} // namespace xci::graphics
