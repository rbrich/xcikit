// Form.cpp created on 2018-06-22, part of XCI toolkit
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

#include "Form.h"
#include <xci/widgets/Label.h>
#include <xci/widgets/TextInput.h>
#include <xci/widgets/Checkbox.h>
#include <utility>

namespace xci {
namespace widgets {


void Form::add_input(const std::string& label, std::string& text_input)
{
    // Label
    auto w_label = std::make_shared<Label>(label);
    add(w_label);
    add_hint(Form::Hint::NextColumn);
    // TextInput
    auto w_text_input = std::make_shared<TextInput>(text_input);
    auto* rawptr = w_text_input.get();
    w_text_input->on_change([rawptr, &text_input](View&) {
        text_input = rawptr->string();
    });
    add(w_text_input);
    add_hint(Form::Hint::NextRow);
}


void Form::add_input(const std::string& label, bool& checkbox)
{
    // Label
    auto w_label = std::make_shared<Label>(label);
    add(w_label);
    add_hint(Form::Hint::NextColumn);
    // Checkbox
    auto w_checkbox = std::make_shared<Checkbox>();
    w_checkbox->set_checked(checkbox);
    auto* rawptr = w_checkbox.get();
    w_checkbox->on_change([rawptr, &checkbox](View&) {
        checkbox = rawptr->checked();
    });
    add(w_checkbox);
    add_hint(Form::Hint::NextRow);
}


void Form::resize(View& view)
{
    // Resize children, compute max_height (vertical space reserved for each line)
    float max_ascent = 0;
    float max_descent = 0;
    for (auto& child : m_child) {
        child->resize(view);
        auto h = child->size().y;
        auto b = child->baseline();
        max_ascent = std::max(b, max_ascent);
        max_descent = std::max(h - b, max_descent);
    }
    const float max_height = max_ascent + max_descent;

    // Position children
    util::Vec2f pos = {0, max_ascent};
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


}} // namespace xci::widgets
