// Widget.h created on 2018-04-23, part of XCI toolkit
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

#ifndef XCI_WIDGETS_WIDGET_H
#define XCI_WIDGETS_WIDGET_H

#include <xci/widgets/Theme.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/View.h>
#include <xci/util/geometry.h>
#include <vector>

namespace xci {
namespace widgets {

using graphics::View;
using graphics::KeyEvent;
using graphics::MousePosEvent;
using graphics::MouseBtnEvent;


class Widget {
public:
    virtual ~Widget() = default;

    void set_theme(Theme& theme) { m_theme = &theme; }
    Theme& theme() const { return *m_theme; }

    void set_position(const util::Vec2f& pos) { m_position = pos; }
    const util::Vec2f& position() const { return m_position; }

    // Events need to be injected into root widget.
    // This can be set up using Bind helper or manually by calling these methods.

    virtual void update(const graphics::View& view) = 0;
    virtual void draw(graphics::View& view) = 0;
    virtual void handle(View& view, const KeyEvent& ev) {}
    virtual void handle(View& view, const MousePosEvent& ev) {}
    virtual void handle(View& view, const MouseBtnEvent& ev) {}

private:
    Theme* m_theme = &Theme::default_theme();
    util::Vec2f m_position;
};


// Manages list of child widgets and forwards all events to them

class Composite: public Widget {
public:
    void add(Widget& child) { m_child.push_back(&child); }

    void update(const graphics::View& view) override;
    void draw(graphics::View& view) override;
    void handle(View& view, const KeyEvent& ev) override;
    void handle(View& view, const MousePosEvent& ev) override;
    void handle(View& view, const MouseBtnEvent& ev) override;

private:
    std::vector<Widget*> m_child;
};


// Connects window to root widget through event callbacks

class Bind {
public:
    Bind(graphics::Window& window, Widget& root);
    ~Bind();

private:
    graphics::Window& m_window;
};


}} // namespace xci::widgets

#endif // XCI_WIDGETS_WIDGET_H
