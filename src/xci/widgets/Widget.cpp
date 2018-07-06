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
#include <xci/util/log.h>
#include <xci/util/rtti.h>
#include <xci/graphics/Window.h>

namespace xci {
namespace widgets {

using graphics::View;
using graphics::Key;
using graphics::Action;
using namespace util::log;


void Widget::partial_dump(std::ostream& stream, const std::string& nl_prefix)
{
    using namespace std;
    stream << util::type_name(typeid(*this))
           << "<" << hex << this << "> "
           << "pos=" << m_position << " "
           << "size=" << m_size << " "
           << "baseline=" << m_baseline << " ";
}


void Composite::add(WidgetPtr child)
{
    if (m_focus.expired())
        m_focus = child;
    m_child.push_back(std::move(child));
}


void Composite::focus_next()
{
    // Check if there is any focusable widget
    if (m_child.empty())
        return;

    // Point iterator to expected position (after currently focused widget)
    auto iter = m_child.begin();
    if (!m_focus.expired()) {
        iter = std::find(m_child.begin(), m_child.end(), m_focus.lock());
        if (iter != m_child.end()) {
            ++iter;
            if (iter == m_child.end())
                iter = m_child.begin();
        }
    }

    // Find first widget which can take focus
    while (!(*iter)->is_tab_focusable()) {
        // try next
        ++iter;
        if (iter == m_child.end())
            iter = m_child.begin();
    }

    m_focus = *iter;
}


void Composite::focus_previous()
{
    // Check if there is any focusable widget
    if (m_child.empty())
        return;

    // Point iterator to expected position (before currently focused widget)
    auto iter = m_child.begin();
    if (!m_focus.expired()) {
        iter = std::find(m_child.begin(), m_child.end(), m_focus.lock());
        if (iter != m_child.end()) {
            if (iter == m_child.begin())
                iter = m_child.begin() + (m_child.size() - 1);
            else
                --iter;
        }
    }

    // Find first widget which can take focus
    while (!(*iter)->is_tab_focusable()) {
        // try previous
        if (iter == m_child.begin())
            iter = m_child.begin() + (m_child.size() - 1);
        else
            --iter;
    }

    m_focus = *iter;
}


bool Composite::contains(const Vec2f& point) const
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


void Composite::draw(View& view, State state)
{
    view.push_offset(position());
    for (auto& child : m_child) {
        state.focused = (m_focus.lock() == child);
        child->draw(view, state);
    }
    view.pop_offset();
}


bool Composite::handle(View& view, const KeyEvent& ev)
{
    // Propagate the event to the focused child
    if (!m_focus.expired())
        if (m_focus.lock()->handle(view, ev))
            return true;

    // Switch focus with Tab, Shift+Tab
    if (ev.action == Action::Press && ev.key == Key::Tab && !m_child.empty()) {
        if (!ev.mod.shift) {
            // tab
            focus_next();
        } else {
            // shift + tab
            focus_previous();
        }
        resize(view);
        view.refresh();
        return true;
    }

    // Not handled
    return false;
}


void Composite::handle(View& view, const CharEvent& ev)
{
    if (!m_focus.expired())
        m_focus.lock()->handle(view, ev);
}


void Composite::handle(View& view, const MousePosEvent& ev)
{
    view.push_offset(position());
    for (auto& child : m_child)
        child->handle(view, ev);
    view.pop_offset();
}


bool Composite::handle(View& view, const MouseBtnEvent& ev)
{
    view.push_offset(position());
    bool handled = false;
    for (auto& child : m_child) {
        // Propagate the event
        handled = child->handle(view, ev);
        if (handled)
            break;
    }
    view.pop_offset();
    return handled;
}


bool Composite::click_focus(View& view, const MouseBtnEvent& ev)
{
    view.push_offset(position());
    bool handled = false;
    auto original_focus = std::move(m_focus);
    for (auto& child : m_child) {
        // Propagate the event
        if (child->click_focus(view, ev)) {
            m_focus = child;
            handled = true;
            break;
        }
    }
    if (original_focus.lock() != m_focus.lock()) {
        resize(view);
        view.refresh();
    }
    view.pop_offset();
    return handled;
}


void Composite::partial_dump(std::ostream& stream, const std::string& nl_prefix)
{
    Widget::partial_dump(stream, nl_prefix);
    for (auto& child : m_child) {
        bool focus = (m_focus.lock() == child);
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


Bind::Bind(graphics::Window& window, Widget& root)
    : m_window(window)
{
    m_size_cb = window.get_size_callback();
    window.set_size_callback([&](View& v) {
        if (m_size_cb)
            m_size_cb(v);
        root.resize(v);
    });

    m_draw_cb = window.get_draw_callback();
    window.set_draw_callback([&](View& v) {
        if (m_draw_cb)
            m_draw_cb(v);
        root.draw(v, {});
    });

    m_key_cb = window.get_key_callback();
    window.set_key_callback([&](View& v, const KeyEvent& e) {
        if (m_key_cb)
            m_key_cb(v, e);
        root.handle(v, e);
    });

    m_char_cb = window.get_char_callback();
    window.set_char_callback([&](View& v, const CharEvent& e) {
        if (m_char_cb)
            m_char_cb(v, e);
        root.handle(v, e);
    });

    m_mpos_cb = window.get_mouse_position_callback();
    window.set_mouse_position_callback([&](View& v, const MousePosEvent& e) {
        if (m_mpos_cb)
            m_mpos_cb(v, e);
        root.handle(v, e);
    });

    m_mbtn_cb = window.get_mouse_button_callback();
    window.set_mouse_button_callback([&](View& v, const MouseBtnEvent& e) {
        if (m_mbtn_cb)
            m_mbtn_cb(v, e);
        root.click_focus(v, e);
        root.handle(v, e);
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


}} // namespace xci::widgets
