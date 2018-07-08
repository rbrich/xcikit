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
    Widget() : m_tab_focusable(false), m_click_focusable(false) {}
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
    float baseline() const { return m_baseline; }

    // Accept keyboard focus by cycling with tab key
    void set_tab_focusable(bool enabled) { m_tab_focusable = enabled; }
    bool is_tab_focusable() const { return m_tab_focusable; }

    // Accept keyboard focus by clicking on the widget
    void set_click_focusable(bool enabled) { m_click_focusable = enabled; }
    bool is_click_focusable() const { return m_click_focusable; }

    // Accept keyboard focus by tab or click
    void set_focusable(bool enabled) { m_tab_focusable = enabled; m_click_focusable = enabled; }

    // Test if point is contained inside widget area
    virtual bool contains(const Vec2f& point) const { return aabb().contains(point); }

    // Events need to be injected into root widget.
    // This can be set up using Bind helper or manually by calling these methods.

    virtual void resize(View& view) {}
    virtual void draw(View& view, State state) = 0;
    virtual bool key_event(View& view, const KeyEvent& ev) { return false; }
    virtual void char_event(View& view, const CharEvent& ev) {}
    virtual void mouse_pos_event(View& view, const MousePosEvent& ev) {}
    virtual bool mouse_button_event(View& view, const MouseBtnEvent& ev) { return false; }
    virtual bool click_focus(View& view, Vec2f pos) { return is_click_focusable() && contains(pos); }
    virtual bool tab_focus(View& view, int& step) { return is_tab_focusable(); }

    // Debug dump
    void dump(std::ostream& stream) { partial_dump(stream, ""); stream << std::endl; }
    virtual void partial_dump(std::ostream& stream, const std::string& nl_prefix);

protected:
    void set_baseline(float baseline) { m_baseline = baseline; }

private:
    Theme* m_theme = &Theme::default_theme();
    Vec2f m_position;
    Vec2f m_size;
    float m_baseline = 0;

    // Flags
    bool m_tab_focusable : 1;
    bool m_click_focusable : 1;
};


using WidgetPtr = std::shared_ptr<Widget>;
using WidgetRef = std::weak_ptr<Widget>;


// Manages list of child widgets and forwards events to them

class Composite: public Widget {
public:
    void add(WidgetPtr child);

    void set_focus(WidgetRef child) { m_focus = std::move(child); }
    WidgetPtr focus() const { return m_focus.lock(); }

    // impl Widget
    bool contains(const Vec2f& point) const override;
    void resize(View& view) override;
    void draw(View& view, State state) override;
    bool key_event(View& view, const KeyEvent& ev) override;
    void char_event(View& view, const CharEvent& ev) override;
    void mouse_pos_event(View& view, const MousePosEvent& ev) override;
    bool mouse_button_event(View& view, const MouseBtnEvent& ev) override;
    bool click_focus(View& view, Vec2f pos) override;
    bool tab_focus(View& view, int& step) override;

    // Debug dump
    void partial_dump(std::ostream& stream, const std::string& nl_prefix) override;

protected:
    std::vector<WidgetPtr> m_child;

private:
    WidgetRef m_focus;  // a child with keyboard focus
};


// Clickable widget mixin
class Clickable {
public:
    using HoverCallback = std::function<void(View&, bool inside)>;
    void on_hover(HoverCallback cb) { m_hover_cb = std::move(cb); }

    using ClickCallback = std::function<void(View&)>;
    void on_click(ClickCallback cb) { m_click_cb = std::move(cb); }

protected:
    // Call from mouse_pos_event like this:
    //
    //     do_hover(view, contains(ev.pos - view.offset()));
    void do_hover(View& view, bool inside);

    // Call from mouse_button_event like this:
    //
    //     if (ev.action == Action::Press && ev.button == MouseButton::Left
    //     && contains(ev.pos - view.offset())) {
    //         do_click(view);
    //         return true;
    //     }
    void do_click(View& view);

    enum class LastHover {
        None,
        Inside,
        Outside,
    };
    LastHover last_hover() const { return m_last_hover; }

private:
    HoverCallback m_hover_cb;
    ClickCallback m_click_cb;
    LastHover m_last_hover {LastHover::None};
};

// Connects window to root widget through event callbacks

class Bind {
public:
    Bind(graphics::Window& window, Widget& root);
    ~Bind();

private:
    graphics::Window& m_window;
    graphics::Window::SizeCallback m_size_cb;
    graphics::Window::DrawCallback m_draw_cb;
    graphics::Window::KeyCallback m_key_cb;
    graphics::Window::CharCallback m_char_cb;
    graphics::Window::MousePosCallback m_mpos_cb;
    graphics::Window::MouseBtnCallback m_mbtn_cb;
};


}} // namespace xci::widgets

#endif // XCI_WIDGETS_WIDGET_H
