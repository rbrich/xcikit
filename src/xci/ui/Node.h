// Node.h created on 2018-04-22, part of XCI toolkit
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

#ifndef XCI_UI_NODE_H
#define XCI_UI_NODE_H

#include <xci/graphics/Window.h>
#include <xci/graphics/View.h>
#include <xci/util/geometry.h>
#include <vector>

namespace xci {
namespace ui {

using graphics::View;
using graphics::KeyEvent;
using graphics::MousePosEvent;
using graphics::MouseBtnEvent;


class Node {
public:
    virtual ~Node() { unbind(); }

    void add(Node& child) { m_child.push_back(&child); }

    void bind(graphics::Window& window);
    void unbind();

    void set_position(const util::Vec2f& pos) { m_pos = pos; }
    const util::Vec2f& position() const { return m_pos; }

protected:
    virtual void handle_resize(View& view);
    virtual void handle_draw(View& view);
    virtual void handle_input(View& view, const KeyEvent& ev);
    virtual void handle_input(View& view, const MousePosEvent& ev);
    virtual void handle_input(View& view, const MouseBtnEvent& ev);

private:
    std::vector<Node*> m_child;
    graphics::Window* m_bound_window = nullptr;
    util::Vec2f m_pos;
};


}} // namespace xci::ui

#endif // XCI_UI_NODE_H
