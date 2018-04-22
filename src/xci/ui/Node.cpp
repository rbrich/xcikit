// Node.cpp created on 2018-04-22, part of XCI toolkit
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

#include "Node.h"
#include <functional>

namespace xci {
namespace ui {

using graphics::View;


void Node::bind(graphics::Window& window)
{
    m_bound_window = &window;
    window.set_size_callback([this](View& v) { handle_resize(v); });
    window.set_draw_callback([this](View& v) { handle_draw(v); });
    window.set_key_callback([this](View& v, const KeyEvent& e) { handle_input(v, e); });
    window.set_mouse_position_callback([this](View& v, const MousePosEvent& e) { handle_input(v, e); });
    window.set_mouse_button_callback([this](View& v, const MouseBtnEvent& e) { handle_input(v, e); });
}


void Node::unbind()
{
    if (m_bound_window == nullptr)
        return;
    m_bound_window->set_size_callback(nullptr);
    m_bound_window->set_draw_callback(nullptr);
    m_bound_window->set_key_callback(nullptr);
    m_bound_window->set_mouse_position_callback(nullptr);
    m_bound_window->set_mouse_button_callback(nullptr);
    m_bound_window = nullptr;
}


void Node::handle_resize(View& view)
{
    for (auto* child : m_child)
        child->handle_resize(view);
}


void Node::handle_draw(View& view)
{
    for (auto* child : m_child)
        child->handle_draw(view);
}


void Node::handle_input(View& view, const KeyEvent& ev)
{
    for (auto* child : m_child)
        child->handle_input(view, ev);
}


void Node::handle_input(View& view, const MousePosEvent& ev)
{
    for (auto* child : m_child)
        child->handle_input(view, ev);
}


void Node::handle_input(View& view, const MouseBtnEvent& ev)
{
    for (auto* child : m_child)
        child->handle_input(view, ev);
}


}} // namespace xci::ui
