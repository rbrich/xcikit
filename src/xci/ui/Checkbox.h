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

#ifndef XCI_UI_CHECKBOX_H
#define XCI_UI_CHECKBOX_H

#include "Node.h"
#include <xci/widgets/Icon.h>

namespace xci {
namespace ui {

using widgets::Theme;


class Checkbox: public widgets::Icon, public Node {
public:
    Checkbox();

    void set_checked(bool checked) { m_checked = checked; }
    bool checked() const { return m_checked; }

    using ChangeCallback = std::function<void(View&)>;
    void set_change_callback(ChangeCallback cb) { m_change_cb = std::move(cb); }

protected:
    void handle_resize(View& view) override { update(view); }
    void handle_draw(View& view) override { draw(view, position()); }
    void handle_input(View& view, const MouseBtnEvent& ev) override;

private:
    bool m_checked = false;
    ChangeCallback m_change_cb;
};


}} // namespace xci::ui

#endif // XCI_UI_CHECKBOX_H
