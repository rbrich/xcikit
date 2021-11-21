// Widget.h created on 2018-04-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_WIDGETS_WIDGET_H
#define XCI_WIDGETS_WIDGET_H

#include <xci/widgets/Theme.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/View.h>
#include <xci/core/geometry.h>
#include <utility>
#include <vector>

namespace xci::widgets {

using graphics::View;
using graphics::KeyEvent;
using graphics::CharEvent;
using graphics::MousePosEvent;
using graphics::MouseBtnEvent;
using graphics::ScrollEvent;
using graphics::ViewportUnits;
using graphics::ViewportCoords;
using graphics::ViewportSize;
using graphics::ViewportRect;


struct State {
    std::chrono::nanoseconds elapsed;
    bool focused = false;
};

class Widget {
public:
    explicit Widget(Theme& theme);
    virtual ~Widget() = default;

    Theme& theme() const { return m_theme; }

    // Set position of widget, relative to the parent
    void set_position(const ViewportCoords& pos) { m_position = pos; }
    const ViewportCoords& position() const { return m_position; }

    // Set size of widget.
    // This may not be respected by actual implementation,
    // but it determines space taken in layout.
    void set_size(const ViewportSize& size) { m_size = size; }
    const ViewportSize& size() const { return m_size; }

    ViewportRect aabb() const { return {m_position, m_size}; }
    ViewportUnits baseline() const { return m_baseline; }

    // Accept keyboard focus by cycling with tab key
    void set_tab_focusable(bool enabled) { m_tab_focusable = enabled; }
    bool is_tab_focusable() const { return m_tab_focusable; }

    // Accept keyboard focus by clicking on the widget
    void set_click_focusable(bool enabled) { m_click_focusable = enabled; }
    bool is_click_focusable() const { return m_click_focusable; }

    // Accept keyboard focus by tab or click
    void set_focusable(bool enabled) { m_tab_focusable = enabled; m_click_focusable = enabled; }

    // Hidden widget doesn't receive update/draw events (when it's a child of Composite)
    void set_hidden(bool hidden) { m_hidden = hidden; }
    void toggle_hidden() { m_hidden = !m_hidden; }
    bool is_hidden() const { return m_hidden; }

    // Test if point is contained inside widget area
    virtual bool contains(const ViewportCoords& point) const { return aabb().contains(point); }

    // Events need to be injected into root widget.
    // This can be set up using Bind helper or manually by calling these methods.

    virtual void resize(View& view) {}
    virtual void update(View& view, State state) {}
    virtual void draw(View& view) = 0;
    virtual bool key_event(View& view, const KeyEvent& ev) { return false; }
    virtual void char_event(View& view, const CharEvent& ev) {}
    virtual void mouse_pos_event(View& view, const MousePosEvent& ev) {}
    virtual bool mouse_button_event(View& view, const MouseBtnEvent& ev) { return false; }
    virtual void scroll_event(View& view, const ScrollEvent& ev) {}
    virtual bool click_focus(View& view, ViewportCoords pos) { return is_click_focusable() && contains(pos); }
    virtual bool tab_focus(View& view, int& step) { return is_tab_focusable(); }

    // Debug dump
    void dump(std::ostream& stream) { partial_dump(stream, ""); stream << std::endl; }
    virtual void partial_dump(std::ostream& stream, const std::string& nl_prefix);

protected:
    void set_baseline(ViewportUnits baseline) { m_baseline = baseline; }

private:
    Theme& m_theme;
    ViewportCoords m_position;
    ViewportSize m_size;
    ViewportUnits m_baseline = 0;

    // Flags
    bool m_tab_focusable : 1;
    bool m_click_focusable : 1;
    bool m_hidden : 1;
};


// Manages list of child widgets and forwards events to them

class Composite: public Widget {
public:
    explicit Composite(Theme& theme) : Widget(theme) {}

    void add(Widget& child);

    void set_focus(Widget& child) { m_focus = &child; }
    void reset_focus() { m_focus = nullptr; }
    Widget* focus() const { return m_focus; }

    // impl Widget
    bool contains(const ViewportCoords& point) const override;

    void resize(View& view) override;
    void update(View& view, State state) override;
    void draw(View& view) override;
    bool key_event(View& view, const KeyEvent& ev) override;
    void char_event(View& view, const CharEvent& ev) override;
    void mouse_pos_event(View& view, const MousePosEvent& ev) override;
    bool mouse_button_event(View& view, const MouseBtnEvent& ev) override;
    void scroll_event(View& view, const ScrollEvent& ev) override;
    bool click_focus(View& view, ViewportCoords pos) override;
    bool tab_focus(View& view, int& step) override;

    // Debug dump
    void partial_dump(std::ostream& stream, const std::string& nl_prefix) override;

protected:
    std::vector<Widget*> m_child;

private:
    Widget* m_focus = nullptr;  // a child with keyboard focus
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
    graphics::Window::UpdateCallback m_update_cb;
    graphics::Window::SizeCallback m_size_cb;
    graphics::Window::DrawCallback m_draw_cb;
    graphics::Window::KeyCallback m_key_cb;
    graphics::Window::CharCallback m_char_cb;
    graphics::Window::MousePosCallback m_mpos_cb;
    graphics::Window::MouseBtnCallback m_mbtn_cb;
    graphics::Window::ScrollCallback m_scroll_cb;
};


} // namespace xci::widgets

#endif // XCI_WIDGETS_WIDGET_H
