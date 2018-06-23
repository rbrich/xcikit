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
#include <utility>
#include <vector>

namespace xci {
namespace widgets {

using graphics::View;
using graphics::KeyEvent;
using graphics::CharEvent;
using graphics::MousePosEvent;
using graphics::MouseBtnEvent;
using util::Vec2f;


struct State {
    bool focused = false;
};

class Widget {
public:
    virtual ~Widget() = default;

    void set_theme(Theme& theme) { m_theme = &theme; }
    Theme& theme() const { return *m_theme; }

    // Set position of widget, relative to the parent
    void set_position(const Vec2f& pos) { m_position = pos; }
    const Vec2f& position() const { return m_position; }

    // Set size of widget.
    // This may not be respected by actual implementation,
    // but it determines space taken in layout.
    void set_size(const Vec2f& size) { m_size = size; }
    const Vec2f& size() const { return m_size; }

    util::Rect_f aabb() const { return {m_position, m_size}; }

    // Is this widget focusable?
    // When true, it has to implement contains().
    virtual bool can_focus() const { return false; }

    // Test if point is contained inside widget area
    virtual bool contains(const Vec2f& point) const { return aabb().contains(point); }

    // Events need to be injected into root widget.
    // This can be set up using Bind helper or manually by calling these methods.

    virtual void resize(View& view) {}
    virtual void draw(View& view, State state) = 0;
    virtual void handle(View& view, const KeyEvent& ev) {}
    virtual void handle(View& view, const CharEvent& ev) {}
    virtual void handle(View& view, const MousePosEvent& ev) {}
    virtual void handle(View& view, const MouseBtnEvent& ev) {}

private:
    Theme* m_theme = &Theme::default_theme();
    Vec2f m_position;
    Vec2f m_size;
};


using WidgetPtr = std::shared_ptr<Widget>;
using WidgetRef = std::weak_ptr<Widget>;


// Manages list of child widgets and forwards events to them

class Composite: public Widget {
public:
    void add(WidgetPtr child);

    void focus(WidgetRef child) { m_focus = std::move(child); }
    WidgetPtr focus() const { return m_focus.lock(); }
    void focus_next();
    void focus_previous();

    bool can_focus() const override;
    bool contains(const Vec2f& point) const override;
    void resize(View& view) override;
    void draw(View& view, State state) override;
    void handle(View& view, const KeyEvent& ev) override;
    void handle(View& view, const CharEvent& ev) override;
    void handle(View& view, const MousePosEvent& ev) override;
    void handle(View& view, const MouseBtnEvent& ev) override;

protected:
    std::vector<WidgetPtr> m_child;

private:
    WidgetRef m_focus;  // a child with keyboard focus
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
