// demo_ui.cpp created on 2018-04-22, part of XCI toolkit
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

#include "MousePosInfo.h"
#include <xci/widgets/Button.h>
#include <xci/widgets/Checkbox.h>
#include <xci/widgets/FpsDisplay.h>
#include <xci/widgets/TextInput.h>
#include <xci/graphics/Window.h>
#include <xci/core/Vfs.h>
#include <xci/core/format.h>
#include <xci/config.h>
#include <random>
#include <cstdlib>

// this brings in all required namespaces
using namespace xci::demo;

int main()
{
    Vfs vfs;
    vfs.mount(XCI_SHARE_DIR);

    Renderer renderer {vfs};
    Window window {renderer};
    window.create({800, 600}, "XCI UI demo");

    Theme theme(renderer);
    if (!theme.load_default())
        return EXIT_FAILURE;

    Composite root {theme};

    // Random color generator
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_int_distribution<int> runi(0, 255);
    auto random_color = [&](){ return Color(runi(re), runi(re), runi(re)); };

    std::list<TextInput> text_inputs;
    for (auto i : {0,1,2,3,4}) {
        text_inputs.emplace_back(theme, "input");
        text_inputs.back().set_position({-1.0f, -0.5f + i * 0.12f});
        root.add(text_inputs.back());
    }

    std::list<Button> buttons;
    for (auto i : {0,1,2,3,4}) {
        buttons.emplace_back(theme, std::to_string(i+1) + ". click me!");
        buttons.back().set_position({-0.2f, -0.5f + i * 0.12f});
        auto* btn_self = &buttons.back();
        buttons.back().on_click([btn_self, &random_color](View& view) {
            view.finish_draw();
            btn_self->set_text_color(random_color());
            btn_self->resize(view);
        });
        root.add(buttons.back());
    }

    std::list<Checkbox> checkboxes;
    for (auto i : {0,1,2,3,4}) {
        checkboxes.emplace_back(theme);
        checkboxes.back().set_text("Checkbox " + std::to_string(i+1));
        checkboxes.back().set_position({0.5f, -0.5f + i * 0.06f});
        root.add(checkboxes.back());
    }

    MousePosInfo mouse_pos_info {theme};
    mouse_pos_info.set_position({-1.2f, 0.9f});
    root.add(mouse_pos_info);

    FpsDisplay fps_display {theme};
    fps_display.set_position({-1.2f, -0.8f});
    root.add(fps_display);

    Bind bind(window, root);
    window.set_refresh_mode(RefreshMode::OnDemand);
    window.display();
    return EXIT_SUCCESS;
}
