// Widget.cpp created on 2018-04-23, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
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
#include <xci/core/rtti.h>
#include <xci/graphics/Window.h>
#include <cassert>

namespace xci::widgets {

using namespace xci::graphics;


Widget::Widget(Theme& theme)
    : m_theme(theme),
      m_tab_focusable(false), m_click_focusable(false)
{}


void Widget::partial_dump(std::ostream& stream, const std::string& nl_prefix)
{
    using namespace std;
    stream << core::type_name(typeid(*this))
           << "<" << hex << this << "> "
           << "pos=" << m_position << " "
           << "size=" << m_size << " "
           << "baseline=" << m_baseline << " ";
}


void Composite::add(Widget& child)
{
    m_child.push_back(&child);
}


bool Composite::contains(const ViewportCoords& point) const
{
    for (auto& child : m_child)
        if (child->contains(point))
            return true;
    return false;
}


void Composite::resize(View& view)
{
    for (auto& child : m_child)
        child->resize(view);
}


void Composite::update(View& view, State state)
{
    for (auto& child : m_child) {
        state.focused = (m_focus == child);
        child->update(view, state);
    }
}


void Composite::draw(View& view)
{
    view.push_offset(position());
    for (auto& child : m_child) {
        child->draw(view);
    }
    view.pop_offset();
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


void Composite::char_event(View& view, const CharEvent& ev)
{
    if (m_focus != nullptr)
        m_focus->char_event(view, ev);
}


void Composite::mouse_pos_event(View& view, const MousePosEvent& ev)
{
    view.push_offset(position());
    for (auto& child : m_child)
        child->mouse_pos_event(view, ev);
    view.pop_offset();
}


bool Composite::mouse_button_event(View& view, const MouseBtnEvent& ev)
{
    view.push_offset(position());
    bool handled = false;
    for (auto& child : m_child) {
        // Propagate the event
        handled = child->mouse_button_event(view, ev);
        if (handled)
            break;
    }
    view.pop_offset();
    return handled;
}


void Composite::scroll_event(View& view, const ScrollEvent& ev)
{
    // TODO: mouse-over widget, enter/exit events, propagate scroll to only one Widget
    for (auto& child : m_child)
        child->scroll_event(view, ev);
}


bool Composite::click_focus(View& view, ViewportCoords pos)
{
    bool handled = false;
    auto* original_focus = m_focus;
    for (auto& child : m_child) {
        // Propagate the event
        if (child->click_focus(view, pos - position())) {
            m_focus = child;
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
            m_focus = *it;
        } else {
            auto it = std::find_if(m_child.rbegin(), m_child.rend(), [&view, &step](auto& w) {
                return w->tab_focus(view, step);
            });
            if (it == m_child.rend())
                return false;
            m_focus = *it;
        }
        resize(view);
        view.refresh();
        return true;
    }

    // Current focus child - propagate event, give it chance to consume the step
    bool res = m_focus->tab_focus(view, step);
    if (res && step == 0)
        return true;

    // Step to next focusable child
    if (step > 0) {
        auto it = std::find(m_child.begin(), m_child.end(), m_focus);
        assert(it != m_child.end());
        it = std::find_if(it+1, m_child.end(), [&view, &step](auto& w) {
            return w->tab_focus(view, step);
        });
        if (it != m_child.end()) {
            m_focus = *it;
            --step;
        } else {
            reset_focus();
        }
    }
    if (step < 0)  {
        auto it = std::find(m_child.rbegin(), m_child.rend(), m_focus);
        assert(it != m_child.rend());
        it = std::find_if(it+1, m_child.rend(), [&view, &step](auto& w) {
            return w->tab_focus(view, step);
        });
        if (it != m_child.rend()) {
            m_focus = *it;
            ++step;
        } else {
            reset_focus();
        }
    }

    resize(view);
    view.refresh();
    return m_focus != nullptr;
}


void Composite::partial_dump(std::ostream& stream, const std::string& nl_prefix)
{
    Widget::partial_dump(stream, nl_prefix);
    for (auto& child : m_child) {
        bool focus = (m_focus == child);
        stream << std::endl << nl_prefix;
        if (child != m_child.back()) {
            // intermediate child
            stream << " " << (focus? ">" : " ") << "├ ";
            child->partial_dump(stream, nl_prefix + "  │ ");
        } else {
            // last child
            stream << " " << (focus? ">" : " ") << "└ ";
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
        root.draw(v);
    });

    m_key_cb = window.key_callback();
    window.set_key_callback([&](View& v, const KeyEvent& e) {
        if (m_key_cb)
            m_key_cb(v, e);
        if (root.key_event(v, e))
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

    m_char_cb = window.char_callback();
    window.set_char_callback([&](View& v, const CharEvent& e) {
        if (m_char_cb)
            m_char_cb(v, e);
        root.char_event(v, e);
    });

    m_mpos_cb = window.mouse_position_callback();
    window.set_mouse_position_callback([&](View& v, const MousePosEvent& e) {
        if (m_mpos_cb)
            m_mpos_cb(v, e);
        root.mouse_pos_event(v, e);
    });

    m_mbtn_cb = window.mouse_button_callback();
    window.set_mouse_button_callback([&](View& v, const MouseBtnEvent& e) {
        if (m_mbtn_cb)
            m_mbtn_cb(v, e);
        root.click_focus(v, e.pos);
        root.mouse_button_event(v, e);
    });

    m_scroll_cb = window.scroll_callback();
    window.set_scroll_callback([&](View& v, const ScrollEvent& e) {
        if (m_scroll_cb)
            m_scroll_cb(v, e);
        root.scroll_event(v, e);
    });
}


Bind::~Bind()
{
    m_window.set_size_callback(m_size_cb);
    m_window.set_draw_callback(m_draw_cb);
    m_window.set_key_callback(m_key_cb);
    m_window.set_char_callback(m_char_cb);
    m_window.set_mouse_position_callback(m_mpos_cb);
    m_window.set_mouse_button_callback(m_mbtn_cb);
}


} // namespace xci::widgets
