// Form.cpp created on 2018-06-22, part of XCI toolkit
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
    // Resize children, compute max_height (vertical space reserved for each line)
    ViewportUnits max_ascent = 0;
    ViewportUnits max_descent = 0;
    for (auto& child : m_child) {
        child->resize(view);
        auto h = child->size().y;
        auto b = child->baseline();
        max_ascent = std::max(b, max_ascent);
        max_descent = std::max(h - b, max_descent);
    }
    const ViewportUnits max_height = max_ascent + max_descent;

    // Position children
    ViewportCoords pos = {0, max_ascent};
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
                    pos.x = 0.5f;
                    break;
                case Hint::NextRow:
                    pos.x = 0;
                    pos.y += max_height + m_margin.y;
                    break;
            }
            hint_it++;
        }

        // Position child
        child->set_position({pos.x, pos.y - child->baseline()});
        pos.x += child->size().x + m_margin.x;
        ++index;
    }
}


void Form::add_hint(size_t child_index, Form::Hint hint)
{
    m_hint.push_back({child_index, hint});
}


} // namespace xci::widgets
