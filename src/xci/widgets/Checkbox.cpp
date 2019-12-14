// Checkbox.cpp created on 2018-04-22, part of XCI toolkit
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

#include "Checkbox.h"


namespace xci::widgets {

using namespace xci::graphics;


Checkbox::Checkbox(Theme& theme)
    : Icon(theme)
{
    set_focusable(true);
    set_icon(IconId::CheckBoxUnchecked);
    on_click([this](View& view) {
        set_checked(!checked());
        resize(view);
    });
    on_hover([this](View& view, bool inside) {
        set_icon_color(inside ?
                       Widget::theme().color(ColorId::Hover) :
                       Widget::theme().color(ColorId::Default));
    });
}


void Checkbox::set_checked(bool checked)
{
    m_checked = checked;
    set_icon(m_checked ? IconId::CheckBoxChecked
                       : IconId::CheckBoxUnchecked);
}


bool Checkbox::key_event(View& view, const KeyEvent& ev)
{
    if (ev.action == Action::Press && ev.key == Key::Enter) {
        do_click(view);
        return true;
    }
    return false;
}


void Checkbox::mouse_pos_event(View& view, const MousePosEvent& ev)
{
    do_hover(view, contains(ev.pos - view.offset()));
}


bool Checkbox::mouse_button_event(View& view, const MouseBtnEvent& ev)
{
    if (ev.action == Action::Press && ev.button == MouseButton::Left
        && contains(ev.pos - view.offset())) {
        do_click(view);
        return true;
    }
    return false;
}


} // namespace xci::widgets
