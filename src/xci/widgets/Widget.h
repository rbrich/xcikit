// Widget.h created on 2018-04-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_WIDGETS_WIDGET_H
#define XCI_WIDGETS_WIDGET_H

#include <xci/widgets/Theme.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/View.h>
#include <xci/math/Vec2.h>
#include <xci/core/mixin.h>
#include <utility>
#include <vector>

namespace xci::widgets {

using namespace xci::graphics;
using namespace xci::graphics::unit_literals;

class Widget;

struct State {
    std::chrono::nanoseconds elapsed;
    bool focused = false;
};

struct FocusChange {
    bool focused;  // true = focus gained, false = focus lost
};


class Widget: private core::NonMovable, private core::NonCopyable {
public:
    explicit Widget(Theme& theme) : m_theme(theme) {}
    virtual ~Widget() = default;

    Theme& theme() const { return m_theme; }

    // Set position of widget, relative to the parent
    void set_position(const VariCoords& pos);
    const FramebufferCoords& position() const { return m_position; }

    // Set size of widget.
    // This may not be respected by actual implementation,
    // but it determines space taken in layout.
    void set_size(const VariSize& size);
    const FramebufferSize& size() const { return m_size; }

    FramebufferRect aabb() const { return {m_position, m_size}; }
    FramebufferPixels baseline() const { return m_baseline; }

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
    virtual bool contains(FramebufferCoords point) const { return aabb().contains(point); }

    // Events need to be injected into root widget.
    // This can be set up using Bind helper or manually by calling these methods.

    virtual void resize(View& view);
    virtual void update(View& view, State state) {}
    virtual void draw(View& view) = 0;
    virtual bool key_event(View& view, const KeyEvent& ev) { return false; }
    virtual void text_input_event(View& view, const TextInputEvent& ev) {}
    virtual void mouse_pos_event(View& view, const MousePosEvent& ev) {}
    virtual bool mouse_button_event(View& view, const MouseBtnEvent& ev) { return false; }
    virtual void scroll_event(View& view, const ScrollEvent& ev) {}
    virtual void focus_change(View& view, const FocusChange& ev) {}

    // Return true if focus was accepted, i.e. this widget or a child of it contains `pos`
    virtual bool click_focus(View& view, FramebufferCoords pos) { return is_click_focusable() && contains(pos); }
    virtual bool tab_focus(View& view, int& step) { return is_tab_focusable(); }

    // Debug dump
    void dump(std::ostream& stream) { partial_dump(stream, ""); stream << std::endl; }
    virtual void partial_dump(std::ostream& stream, const std::string& nl_prefix) const;

protected:
    void set_baseline(FramebufferPixels baseline) { m_baseline = baseline; }

private:
    Theme& m_theme;
    VariCoords m_position_request;
    VariSize m_size_request;
    FramebufferCoords m_position;
    FramebufferSize m_size;
    FramebufferPixels m_baseline = 0;

    // Flags
    bool m_tab_focusable : 1 = false;
    bool m_click_focusable : 1 = false;
    bool m_hidden : 1 = false;
};


/// Manages list of child widgets and forwards events to them
class Composite: public Widget {
public:
    explicit Composite(Theme& theme) : Widget(theme) {}

    void add_child(Widget& child) { m_child.push_back(&child); }
    void remove_child(size_t child_index) { m_child.erase(m_child.begin() + child_index); m_focus = nullptr; }
    void replace_child(size_t child_index, Widget& new_child) { m_child[child_index] = &new_child; }
    void clear_children() { m_child.clear(); m_focus = nullptr; }
    size_t num_children() const { return m_child.size(); }

    void set_focus(Widget* child) { m_focus = child; }  // does not emit focus_change event
    void set_focus(View& view, Widget* child);
    bool has_focus(const Widget* child) const { return m_focus == child; }
    Widget* focus() const { return m_focus; }

    // impl Widget
    bool contains(FramebufferCoords point) const override;

    void resize(View& view) override;
    void update(View& view, State state) override;
    void draw(View& view) override;
    bool key_event(View& view, const KeyEvent& ev) override;
    void text_input_event(View& view, const TextInputEvent& ev) override;
    void mouse_pos_event(View& view, const MousePosEvent& ev) override;
    bool mouse_button_event(View& view, const MouseBtnEvent& ev) override;
    void scroll_event(View& view, const ScrollEvent& ev) override;
    bool click_focus(View& view, FramebufferCoords pos) override;
    bool tab_focus(View& view, int& step) override;

    // Debug dump
    void partial_dump(std::ostream& stream, const std::string& nl_prefix) const override;

protected:
    std::vector<Widget*> m_child;

private:
    Widget* m_focus = nullptr;  // a child with keyboard focus
};


/// Clickable widget mixin
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


class Padded {
public:
    void set_padding(VariUnits padding) { m_padding = {padding, padding}; }
    void set_padding(VariSize padding) { m_padding = padding; }

protected:
    void apply_padding(FramebufferRect& rect, const View& view) const {
        rect.enlarge(padding_fb(view));
    }

    FramebufferSize padding_fb(const View& view) const {
        return view.to_fb(m_padding);
    }

private:
    VariSize m_padding = {0.7_vp, 0.7_vp};
};


/// Connects window to root widget through event callbacks
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
    graphics::Window::TextInputCallback m_text_cb;
    graphics::Window::MousePosCallback m_mpos_cb;
    graphics::Window::MouseBtnCallback m_mbtn_cb;
    graphics::Window::ScrollCallback m_scroll_cb;
};


} // namespace xci::widgets

#endif // XCI_WIDGETS_WIDGET_H
