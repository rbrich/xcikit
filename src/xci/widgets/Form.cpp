// Form.cpp created on 2018-06-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Form.h"
#include <utility>

namespace xci::widgets {


void Form::add_input(const std::string& label, std::string& text_input)
{
    // Label
    m_labels.emplace_back(theme(), label);
    add(m_labels.back());
    add_hint(Form::Hint::NextColumn);
    // TextInput
    m_text_inputs.emplace_back(theme(), text_input);
    auto* p_text_input = &m_text_inputs.back();
    m_text_inputs.back().on_change([p_text_input, &text_input](View&) {
        text_input = p_text_input->string();
    });
    add(m_text_inputs.back());
    add_hint(Form::Hint::NextRow);
}


void Form::add_input(const std::string& label, bool& checkbox)
{
    // Label
    m_labels.emplace_back(theme(), label);
    add(m_labels.back());
    add_hint(Form::Hint::NextColumn);
    // Checkbox
    m_checkboxes.emplace_back(theme());
    m_checkboxes.back().set_checked(checkbox);
    auto* p_checkbox = &m_checkboxes.back();
    m_checkboxes.back().on_change([p_checkbox, &checkbox]() {
        checkbox = p_checkbox->checked();
    });
    add(m_checkboxes.back());
    add_hint(Form::Hint::NextRow);
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
