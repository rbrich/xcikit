// Button.h created on 2018-04-22, part of XCI toolkit
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

#ifndef XCI_UI_BUTTON_H
#define XCI_UI_BUTTON_H

#include "Node.h"
#include <xci/widgets/Button.h>
#include <functional>

namespace xci {
namespace ui {

using widgets::Theme;


class Button: public widgets::Button, public Node {
public:
    explicit Button(const std::string &string, Theme& theme = Theme::default_theme())
        : widgets::Button(string, theme) {}

    using ClickCallback = std::function<void(View&)>;
    void set_click_callback(ClickCallback cb) { m_click_cb = std::move(cb); }

protected:
    void handle_resize(View& view) override { update(view); }
    void handle_draw(View& view) override { draw(view, position()); }
    void handle_input(View& view, const MouseBtnEvent& ev) override;

private:
    ClickCallback m_click_cb;
};


}} // namespace xci::ui

#endif // XCI_UI_BUTTON_H
