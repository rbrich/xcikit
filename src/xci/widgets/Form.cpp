// Form.cpp created on 2018-06-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Form.h"
#include <utility>

namespace xci::widgets {


void Form::clear()
{
    clear_children();
    m_hint.clear();
    m_labels.clear();
    m_text_inputs.clear();
    m_checkboxes.clear();
    m_spinners.clear();
    m_color_pickers.clear();
}


Label& Form::add_label(const std::string& label)
{
    m_labels.emplace_back(theme(), label);
    add_child(m_labels.back());
    add_hint(Form::Hint::NextRow);
    return m_labels.back();
}


auto Form::add_input(const std::string& label, std::string& text) -> std::pair<Label&, TextInput&>
{
    // Label
    m_labels.emplace_back(theme(), label);
    auto& w_label = m_labels.back();
    add_child(w_label);
    add_hint(Form::Hint::NextColumn);
    // TextInput
    m_text_inputs.emplace_back(theme(), text);
    auto& w_text_input = m_text_inputs.back();
    w_text_input.on_change([&text](TextInput& o) {
        text = o.string();
    });
    add_child(w_text_input);
    add_hint(Form::Hint::NextRow);
    return {w_label, w_text_input};
}


auto Form::add_input(const std::string& label, bool& value) -> std::pair<Label&, Checkbox&>
{
    // Label
    m_labels.emplace_back(theme(), label);
    auto& w_label = m_labels.back();
    add_child(w_label);
    add_hint(Form::Hint::NextColumn);
    // Checkbox
    m_checkboxes.emplace_back(theme());
    auto& w_checkbox = m_checkboxes.back();
    w_checkbox.set_checked(value);
    w_checkbox.on_change([&value](Checkbox& o) {
        value = o.checked();
    });
    add_child(w_checkbox);
    add_hint(Form::Hint::NextRow);
    return {w_label, w_checkbox};
}


auto Form::add_input(const std::string& label, float& value) -> std::pair<Label&, Spinner&>
{
    // Label
    m_labels.emplace_back(theme(), label);
    auto& w_label = m_labels.back();
    add_child(w_label);
    add_hint(Form::Hint::NextColumn);
    // Checkbox
    m_spinners.emplace_back(theme(), value);
    auto& w_spinner = m_spinners.back();
    w_spinner.on_change([&value](Spinner& o) {
        value = o.value();
    });
    add_child(w_spinner);
    add_hint(Form::Hint::NextRow);
    return {w_label, w_spinner};
}


auto Form::add_input(const std::string& label, Color& color) -> std::pair<Label&, ColorPicker&>
{
    // Label
    m_labels.emplace_back(theme(), label);
    auto& w_label = m_labels.back();
    add_child(w_label);
    add_hint(Form::Hint::NextColumn);
    // ColorPicker
    m_color_pickers.emplace_back(theme(), color);
    auto& w_color_picker = m_color_pickers.back();
    w_color_picker.on_change([&color](ColorPicker& o) {
        color = o.color();
    });
    add_child(w_color_picker);
    add_hint(Form::Hint::NextRow);
    return {w_label, w_color_picker};
}


void Form::resize(View& view)
{
    Composite::resize(view);
    view.finish_draw();

    // Resize children, compute max_height (vertical space reserved for each line)
    FramebufferPixels max_ascent = 0;
    FramebufferPixels max_descent = 0;
    for (auto& child : m_child) {
        auto h = child->size().y;
        auto b = child->baseline();
        max_ascent = std::max(b, max_ascent);
        max_descent = std::max(h - b, max_descent);
    }
    const auto max_height = max_ascent + max_descent;

    // Position children
    FramebufferCoords pos = {0, max_ascent};
    const auto margin = view.to_fb(m_margin);
    std::sort(m_hint.begin(), m_hint.end());
    size_t index = 0;
    auto hint_it = m_hint.cbegin();
    for (auto& child : m_child) {
        // Find hint by index
        while (hint_it != m_hint.cend() && hint_it->child_index < index)
            hint_it++;

        // Apply hints
        while (hint_it != m_hint.cend() && hint_it->child_index == index) {
            switch (hint_it->hint) {
                case Hint::None:
                    break;
                case Hint::NextColumn:
                    pos.x = view.vp_to_fb(25_vp);
                    break;
                case Hint::NextRow:
                    pos.x = 0;
                    pos.y += max_height + margin.y;
                    break;
            }
            hint_it++;
        }

        // Position child
        child->set_position({pos.x, pos.y - child->baseline()});
        pos.x += child->size().x + margin.x;
        ++index;
    }
}


void Form::add_hint(size_t child_index, Form::Hint hint)
{
    m_hint.push_back({child_index, hint});
}


} // namespace xci::widgets
