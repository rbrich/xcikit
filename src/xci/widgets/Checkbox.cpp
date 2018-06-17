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


namespace xci {
namespace widgets {

using namespace xci::graphics;


Checkbox::Checkbox()
{
    set_icon(IconId::CheckBoxUnchecked);
}


void Checkbox::set_checked(bool checked)
{
    m_checked = checked;
    set_icon(m_checked ? IconId::CheckBoxChecked
                       : IconId::CheckBoxUnchecked);
}


void Checkbox::handle(View& view, const KeyEvent& ev)
{
    if (ev.action == Action::Press && ev.key == Key::Enter) {
        set_checked(!m_checked);
        resize(view);
        view.refresh();
        if (m_change_cb)
            m_change_cb(view);
    }
}


void Checkbox::handle(View& view, const MouseBtnEvent& ev)
{
    if (ev.action == Action::Press && ev.button == MouseButton::Left) {
        if (contains(ev.pos)) {
            set_checked(!m_checked);
            resize(view);
            view.refresh();
            if (m_change_cb)
                m_change_cb(view);
        }
    }
}


}} // namespace xci::widgets
