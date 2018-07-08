// Checkbox.h created on 2018-04-22, part of XCI toolkit
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

#ifndef XCI_WIDGETS_CHECKBOX_H
#define XCI_WIDGETS_CHECKBOX_H

#include <xci/widgets/Icon.h>
#include <xci/graphics/View.h>
#include <xci/graphics/Window.h>

namespace xci {
namespace widgets {

using widgets::Theme;
using graphics::View;
using graphics::MouseBtnEvent;


class Checkbox: public Icon, public Clickable {
public:
    Checkbox();

    void set_checked(bool checked);
    bool checked() const { return m_checked; }

    bool key_event(View& view, const KeyEvent& ev) override;
    void mouse_pos_event(View& view, const MousePosEvent& ev) override;
    bool mouse_button_event(View& view, const MouseBtnEvent& ev) override;

private:
    bool m_checked = false;
};


}} // namespace xci::widgets

#endif // XCI_WIDGETS_CHECKBOX_H
