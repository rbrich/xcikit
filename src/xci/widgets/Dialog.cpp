// Dialog.cpp created on 2023-12-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Dialog.h"

namespace xci::widgets {


Dialog::Dialog(Theme& theme)
        : Widget(theme)
{
    set_focusable(true);
    m_layout.set_default_font(&theme.base_font());
}


void Dialog::create_items_from_spans()
{
    clear_items();
    m_items.reserve(m_layout.span_names().size());
    for (const auto& span_name : m_layout.span_names()) {
        add_item(span_name);
    }
}


auto Dialog::get_item(std::string span_name) -> Item*
{
    auto it = std::find_if(m_items.begin(), m_items.end(),
                [&span_name](const Item& item) { return item.span_name == span_name; });
    if (it == m_items.end())
        return nullptr;
    return &*it;
}


void Dialog::resize(View& view)
{
    view.finish_draw();
    TextMixin::_resize(view);
    for (const auto& item : m_items) {
        auto* span = m_layout.get_span(item.span_name);
        if (span)
            m_styles[item.normal_style].apply(*span);
    }
    auto rect = m_layout.bbox();
    apply_padding(rect, view);
    set_size(rect.size());
    set_baseline(-rect.y);
    Widget::resize(view);
}


void Dialog::update(View& view, State state)
{
    TextMixin::_update(view);
}


void Dialog::draw(View& view)
{
    auto pop_offset = view.push_offset(position() - m_layout.bbox().top_left() + padding_fb(view));
    TextMixin::_draw(view, {});
}


bool Dialog::key_event(View& view, const KeyEvent& ev)
{
    if (ev.action != Action::Press && ev.action != Action::Release)
        return false;  // ignore Repeat
    // Key press - only when no mouse selection is active
    if (ev.action == Action::Press && m_selection_type == SelectionType::Click)
        return false;
    // Key release - only after key pressed
    if (ev.action == Action::Release && m_selection_type != SelectionType::KeyPress)
        return false;
    // Find corresponding Item
    Item* found_item = nullptr;
    for (Item& item : m_items) {
        if (ev.key == item.key || ev.key == item.alternative_key()) {
            found_item = &item;
            break;
        }
    }
    if (!found_item)
        return false;
    const auto found_idx = unsigned(found_item - &m_items.front());
    // Key press - select span
    if (ev.action == Action::Press) {
        m_selected_idx = found_idx;
        m_selection_type = SelectionType::KeyPress;
        auto* span = m_layout.get_span(found_item->span_name);
        if (span != nullptr)
            m_styles[found_item->active_style].apply(*span);
        return true;
    }
    // Key release - if the key corresponds to selected item
    if (ev.action == Action::Release && found_idx == m_selected_idx) {
        // Activate
        if (m_activation_cb)
            m_activation_cb(view, *found_item);
        clear_selection();
        return true;
    }
    return false;
}


void Dialog::mouse_pos_event(View& view, const MousePosEvent& ev)
{
    auto pop_offset = view.push_offset(position() - m_layout.bbox().top_left() + padding_fb(view));
    handle_mouse_move(ev.pos - view.offset());
    Widget::mouse_pos_event(view, ev);
}


bool Dialog::mouse_button_event(View& view, const MouseBtnEvent& ev)
{
    auto pop_offset = view.push_offset(position() - m_layout.bbox().top_left() + padding_fb(view));
    if (ev.action == Action::Press && handle_mouse_press(ev.button, ev.pos - view.offset()))
        return true;
    if (ev.action == Action::Release && handle_mouse_release(view, ev.button, ev.pos - view.offset()))
        return true;
    return Widget::mouse_button_event(view, ev);
}


void Dialog::handle_mouse_move(const FramebufferCoords& coords)
{
    // We're not in key-pressed state
    if (m_selection_type == SelectionType::KeyPress)
        return;
    // Moving with pressed button
    if (m_selection_type == SelectionType::Click) {
        if (m_selected_idx != none_selected) {
            auto* span = m_layout.get_span(m_items[m_selected_idx].span_name);
            if (span != nullptr && !span->contains(coords)) {
                clear_selection();
            }
        }
        return;
    }
    // Look for an item under mouse cursor
    clear_selection();
    for (Item& item : m_items) {
        auto* span = m_layout.get_span(item.span_name);
        if (span != nullptr && span->contains(coords)) {
            m_selected_idx = unsigned(&item - &m_items.front());
            m_selection_type = SelectionType::Hover;
            m_styles[item.hover_style].apply(*span);
            return;
        }
    }
}


bool Dialog::handle_mouse_press(MouseButton button, const FramebufferCoords& coords)
{
    // Left button pressed and not in key press state
    if (button != MouseButton::Left || m_selection_type == SelectionType::KeyPress)
        return false;
    // Look for an item under mouse cursor
    for (Item& item : m_items) {
        auto* span = m_layout.get_span(item.span_name);
        if (span != nullptr && span->contains(coords)) {
            m_selected_idx = unsigned(&item - &m_items.front());
            m_selection_type = SelectionType::Click;
            m_styles[item.active_style].apply(*span);
            return true;
        }
    }
    return false;
}


bool Dialog::handle_mouse_release(View& view, MouseButton button, const FramebufferCoords& coords)
{
    // Left button pressed and we're in mouse press state
    if (button != MouseButton::Left || m_selection_type != SelectionType::Click)
        return false;
    // An item is selected
    if (m_selected_idx != none_selected) {
        auto* span = m_layout.get_span(m_items[m_selected_idx].span_name);
        if (span != nullptr && span->contains(coords)) {
            // Activate
            if (m_activation_cb)
                m_activation_cb(view, m_items[m_selected_idx]);
            clear_selection();
            return true;
        }
    }
    return false;
}


void Dialog::clear_selection()
{
    if (m_selected_idx == none_selected)
        return;
    // Get span
    const auto& item = m_items[m_selected_idx];
    auto* span = m_layout.get_span(item.span_name);
    // Reset span style
    if (span != nullptr) {
        m_styles[item.normal_style].apply(*span);
    }
    // Clear selection
    m_selected_idx = none_selected;
    m_selection_type = SelectionType::None;
}


auto Dialog::Item::alternative_key() const -> graphics::Key
{
    if (key >= Key::Num0 && key <= Key::Num9)
        return Key(int(key) - int(Key::Num0) + int(Key::Keypad0));
    return Key::Unknown;
}


void Dialog::SpanStyle::apply(text::layout::Span& span) const
{
    span.adjust_color(color);
}


} // namespace xci::widgets
