// Widget.cpp created on 2018-04-23, part of XCI toolkit
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

#include "Widget.h"

namespace xci {
namespace widgets {

using graphics::View;


void Composite::update(View& view)
{
    for (auto& child : m_child)
        child->update(view);
}


void Composite::draw(View& view)
{
    for (auto& child : m_child)
        child->draw(view);
}


void Composite::handle(View& view, const KeyEvent& ev)
{
    for (auto& child : m_child)
        child->handle(view, ev);
}


void Composite::handle(View& view, const MousePosEvent& ev)
{
    for (auto& child : m_child)
        child->handle(view, ev);
}


void Composite::handle(View& view, const MouseBtnEvent& ev)
{
    for (auto& child : m_child)
        child->handle(view, ev);
}


Bind::Bind(graphics::Window& window, Widget& root)
    : m_window(window)
{
    window.set_size_callback([&](View& v) { root.update(v); });
    window.set_draw_callback([&](View& v) { root.draw(v); });
    window.set_key_callback([&](View& v, const KeyEvent& e) { root.handle(v, e); });
    window.set_mouse_position_callback([&](View& v, const MousePosEvent& e) { root.handle(v, e); });
    window.set_mouse_button_callback([&](View& v, const MouseBtnEvent& e) { root.handle(v, e); });
}


Bind::~Bind()
{
    m_window.set_size_callback(nullptr);
    m_window.set_draw_callback(nullptr);
    m_window.set_key_callback(nullptr);
    m_window.set_mouse_position_callback(nullptr);
    m_window.set_mouse_button_callback(nullptr);
}


}} // namespace xci::widgets
