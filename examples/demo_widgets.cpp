// demo_widget.cpp created on 2018-03-20, part of XCI toolkit
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

#include <xci/widgets/Theme.h>
#include <xci/widgets/Button.h>
#include <xci/widgets/Icon.h>
#include <xci/text/Text.h>
#include <xci/graphics/Window.h>
#include <xci/core/Vfs.h>
#include <xci/core/format.h>
#include <xci/core/log.h>
#include <xci/config.h>
#include <cstdlib>

using namespace xci::widgets;
using namespace xci::text;
using namespace xci::graphics;
using namespace xci::core;
using namespace xci::core::log;

int main()
{
    Vfs::default_instance().mount(XCI_SHARE_DIR);

    Window& window = Window::default_instance();
    window.create({800, 600}, "XCI widgets demo");

    if (!Theme::load_default_theme())
        return EXIT_FAILURE;

    Button button_default("Default button");
    button_default.set_position({0, -0.2f});

    Button button_styled("Styled button");
    button_styled.set_font_size(0.07);
    button_styled.set_padding(0.05);
    button_styled.set_decoration_color(Color(10, 20, 100), Color(20, 50, 150));
    button_styled.set_text_color(Color(255, 255, 50));

    Icon checkbox;
    checkbox.set_position({0, 0.4f});
    checkbox.set_icon(IconId::CheckBoxChecked);
    checkbox.set_text("Checkbox");
    checkbox.set_font_size(0.08);
    checkbox.set_color(Color(150, 200, 200));
    bool checkbox_state = true;

    window.set_size_callback([&](View& view) {
        //view.set_debug_flag(View::Debug::WordBasePoint);
        //view.set_debug_flag(View::Debug::PageBBox);
        button_default.resize(view);
        button_styled.set_outline_thickness(view.size_to_viewport(1_sc));
        button_styled.resize(view);
        checkbox.resize(view);
    });

    window.set_draw_callback([&](View& view) {
        button_default.draw(view, {});
        button_styled.draw(view, {});
        checkbox.draw(view, {});
    });

    window.set_mouse_button_callback([&](View& view, const MouseBtnEvent& ev) {
        if (ev.action == Action::Press && ev.button == MouseButton::Left) {
            log_debug("checkbox mouse {}", ev.pos - view.offset());
            log_debug("checkbox bbox {}", checkbox.aabb());
            if (checkbox.contains(ev.pos - view.offset())) {
                checkbox_state = !checkbox_state;
                log_debug("checkbox state {}", checkbox_state);
                checkbox.set_icon(checkbox_state ? IconId::CheckBoxChecked
                                                 : IconId::CheckBoxUnchecked);
                checkbox.resize(view);
                view.refresh();
            }
        }
    });

    window.set_mouse_position_callback([&checkbox](View& view, const MousePosEvent& ev) {
        if (checkbox.contains(ev.pos - view.offset())) {
            checkbox.set_color(Color::White());
        } else {
            checkbox.set_color(Color(150, 200, 200));
        }
        checkbox.resize(view);
        view.refresh();
    });

    window.display();
    return EXIT_SUCCESS;
}
