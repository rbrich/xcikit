// Widget.cpp created on 2018-04-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Widget.h"
#include <xci/core/rtti.h>
#include <xci/graphics/Window.h>
#include <range/v3/algorithm/any_of.hpp>
#include <cassert>

namespace xci::widgets {

using namespace xci::graphics;
using ranges::cpp20::any_of;


void Widget::set_position(const VariCoords& pos)
{
    m_position_request = pos;
    if (pos.x.type() == VariUnits::Framebuffer && pos.y.type() == VariUnits::Framebuffer)
        m_position = {pos.x.as_framebuffer(), pos.y.as_framebuffer()};
}


void Widget::set_size(const VariSize& size)
{
    m_size_request = size;
    if (size.x.type() == VariUnits::Framebuffer && size.y.type() == VariUnits::Framebuffer)
        m_size = {size.x.as_framebuffer(), size.y.as_framebuffer()};
}


void Widget::resize(View& view)
{
    m_position = view.to_fb(m_position_request);
    m_size = view.to_fb(m_size_request);
}


void Widget::partial_dump(std::ostream& stream, const std::string& nl_prefix) const
{
    stream << core::type_name(typeid(*this))
           << "<" << std::hex << this << "> "
           << "pos=" << m_position << " "
           << "size=" << m_size << " "
           << "baseline=" << m_baseline << " ";
}


// -------------------------------------------------------------------------------------------------


void Composite::set_focus(View& view, Widget* child)
{
    Widget* prev_focus = m_focus;
    m_focus = child;
    if (prev_focus)
        prev_focus->focus_change(view, {false});
    if (m_focus)
        m_focus->focus_change(view, {true});
}


bool Composite::contains(FramebufferCoords point) const
{
    return any_of(m_child, [&point](const Widget* child){ return child->contains(point); });
}


void Composite::resize(View& view)
{
    Widget::resize(view);
    for (auto& child : m_child)
        child->resize(view);
}


void Composite::update(View& view, State state)
{
    for (auto& child : m_child) {
        if (!child->is_hidden()) {
            state.focused = has_focus(child);
            child->update(view, state);
        }
    }
}


void Composite::draw(View& view)
{
    auto pop_offset = view.push_offset(position());
    for (auto& child : m_child) {
        if (!child->is_hidden())
            child->draw(view);
    }
}


bool Composite::key_event(View& view, const KeyEvent& ev)
{
    // Propagate the event to the focused child
    if (m_focus != nullptr) {
        if (m_focus->key_event(view, ev))
            return true;
    }

    // Not handled
    return false;
}


void Composite::text_input_event(View& view, const TextInputEvent& ev)
{
    if (m_focus != nullptr)
        m_focus->text_input_event(view, ev);
}


void Composite::mouse_pos_event(View& view, const MousePosEvent& ev)
{
    auto pop_offset = view.push_offset(position());
    for (auto& child : m_child) {
        if (!child->is_hidden())
            child->mouse_pos_event(view, ev);
    }
}


bool Composite::mouse_button_event(View& view, const MouseBtnEvent& ev)
{
    auto pop_offset = view.push_offset(position());
    for (auto& child : m_child) {
        if (child->is_hidden())
            continue;
        // Propagate the event
        bool handled = child->mouse_button_event(view, ev);
        if (handled)
            return true;
    }
    return false;
}


void Composite::scroll_event(View& view, const ScrollEvent& ev)
{
    for (auto& child : m_child) {
        if (!child->is_hidden())
            child->scroll_event(view, ev);
    }
}


bool Composite::click_focus(View& view, FramebufferCoords pos)
{
    bool handled = false;
    const auto* original_focus = m_focus;
    for (auto* child : m_child) {
        if (child->is_hidden())
            continue;
        // Propagate the event
        if (child->click_focus(view, pos - position())) {
            set_focus(view, child);
            handled = true;
            break;
        }
    }
    if (original_focus != m_focus) {
        resize(view);
        view.refresh();
    }
    return handled;
}


bool Composite::tab_focus(View& view, int& step)
{
    // No children at all - early exit (this is just an optimization)
    if (m_child.empty())
        return false;

    // No focus child - change to first or last focusable child
    if (m_focus == nullptr) {
        if (step >= 0) {
            auto it = std::find_if(m_child.begin(), m_child.end(), [&view, &step](auto& w) {
                return w->tab_focus(view, step);
            });
            if (it == m_child.end())
                return false;
            set_focus(view, *it);
        } else {
            auto it = std::find_if(m_child.rbegin(), m_child.rend(), [&view, &step](auto& w) {
                return w->tab_focus(view, step);
            });
            if (it == m_child.rend())
                return false;
            set_focus(view, *it);
        }
        resize(view);
        view.refresh();
        return true;
    }

    // Current focus child - propagate event, give it chance to consume the step
    if (m_focus->tab_focus(view, step) && step == 0)
        return true;

    // Step to next focusable child
    if (step > 0) {
        auto it = std::find(m_child.begin(), m_child.end(), m_focus);
        assert(it != m_child.end());
        it = std::find_if(it+1, m_child.end(), [&view, &step](auto& w) {
            return w->tab_focus(view, step);
        });
        if (it != m_child.end()) {
            set_focus(view, *it);
            --step;
        } else {
            set_focus(view, nullptr);
        }
    }
    if (step < 0)  {
        auto it = std::find(m_child.rbegin(), m_child.rend(), m_focus);
        assert(it != m_child.rend());
        it = std::find_if(it+1, m_child.rend(), [&view, &step](auto& w) {
            return w->tab_focus(view, step);
        });
        if (it != m_child.rend()) {
            set_focus(view, *it);
            ++step;
        } else {
            set_focus(view, nullptr);
        }
    }

    resize(view);
    view.refresh();
    return m_focus != nullptr;
}


void Composite::partial_dump(std::ostream& stream, const std::string& nl_prefix) const
{
    Widget::partial_dump(stream, nl_prefix);
    for (const auto* child : m_child) {
        const bool focused = has_focus(child);
        stream << std::endl << nl_prefix;
        if (child != m_child.back()) {
            // intermediate child
            stream << " " << (focused? ">" : " ") << "├ ";
            child->partial_dump(stream, nl_prefix + "  │ ");
        } else {
            // last child
            stream << " " << (focused? ">" : " ") << "└ ";
            child->partial_dump(stream, nl_prefix + "    ");
        }
    }
}


void Clickable::do_hover(View& view, bool inside)
{
    if ((inside && m_last_hover == LastHover::Inside)
    || (!inside && m_last_hover == LastHover::Outside))
        return;
    if (m_hover_cb) {
        m_hover_cb(view, inside);
        view.refresh();
    }
    m_last_hover = inside ? LastHover::Inside : LastHover::Outside;
}


void Clickable::do_click(View& view)
{
    if (m_click_cb) {
        m_click_cb(view);
        view.refresh();
    }
}


Bind::Bind(graphics::Window& window, Widget& root)
    : m_window(window)
{
    m_update_cb = window.update_callback();
    window.set_update_callback([&](View& v, std::chrono::nanoseconds t) {
        if (m_update_cb)
            m_update_cb(v, t);
        if (!root.is_hidden())
            root.update(v, State{ .elapsed = t });
    });

    m_size_cb = window.size_callback();
    window.set_size_callback([&](View& v) {
        if (m_size_cb)
            m_size_cb(v);
        root.resize(v);
    });

    m_draw_cb = window.draw_callback();
    window.set_draw_callback([&](View& v) {
        if (m_draw_cb)
            m_draw_cb(v);
        if (!root.is_hidden())
            root.draw(v);
    });

    m_key_cb = window.key_callback();
    window.set_key_callback([&](View& v, const KeyEvent& e) {
        if (m_key_cb)
            m_key_cb(v, e);
        if (root.is_hidden() || root.key_event(v, e))
            return;
        // Switch focus with Tab, Shift+Tab
        if (e.action == Action::Press && e.key == Key::Tab) {
            int step = e.mod.shift ? -1 : 1;
            // When root widget returns false, it means that either
            // - there is no focusable widget, or
            // - the focus cycled to initial state (nothing is focused).
            // In the second case, call tab_focus again to skip
            // the initial state when cycling with Tab key.
            (root.tab_focus(v, step) || root.tab_focus(v, step));
        }
    });

    m_text_cb = window.text_input_callback();
    window.set_text_input_callback([&](View& v, const TextInputEvent& e) {
        if (m_text_cb)
            m_text_cb(v, e);
        if (!root.is_hidden())
            root.text_input_event(v, e);
    });

    m_mpos_cb = window.mouse_position_callback();
    window.set_mouse_position_callback([&](View& v, const MousePosEvent& e) {
        if (m_mpos_cb)
            m_mpos_cb(v, e);
        if (!root.is_hidden())
            root.mouse_pos_event(v, e);
    });

    m_mbtn_cb = window.mouse_button_callback();
    window.set_mouse_button_callback([&](View& v, const MouseBtnEvent& e) {
        if (m_mbtn_cb)
            m_mbtn_cb(v, e);
        if (!root.is_hidden()) {
            root.click_focus(v, e.pos);
            root.mouse_button_event(v, e);
        }
    });

    m_scroll_cb = window.scroll_callback();
    window.set_scroll_callback([&](View& v, const ScrollEvent& e) {
        if (m_scroll_cb)
            m_scroll_cb(v, e);
        if (!root.is_hidden())
            root.scroll_event(v, e);
    });
}


Bind::~Bind()
{
    m_window.set_size_callback(m_size_cb);
    m_window.set_draw_callback(m_draw_cb);
    m_window.set_key_callback(m_key_cb);
    m_window.set_text_input_callback(m_text_cb);
    m_window.set_mouse_position_callback(m_mpos_cb);
    m_window.set_mouse_button_callback(m_mbtn_cb);
}


} // namespace xci::widgets
