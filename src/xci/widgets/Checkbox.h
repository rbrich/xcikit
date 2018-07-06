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


class Checkbox: public widgets::Icon {
public:
    Checkbox();

    void set_checked(bool checked);
    bool checked() const { return m_checked; }

    using ChangeCallback = std::function<void(View&)>;
    void on_change(ChangeCallback cb) { m_change_cb = std::move(cb); }

    bool handle(View& view, const KeyEvent& ev) override;
    bool handle(View& view, const MouseBtnEvent& ev) override;

private:
    bool m_checked = false;
    ChangeCallback m_change_cb;
};


}} // namespace xci::widgets

#endif // XCI_WIDGETS_CHECKBOX_H
