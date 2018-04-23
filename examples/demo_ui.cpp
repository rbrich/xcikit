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

#include <xci/widgets/Button.h>
#include <xci/widgets/Checkbox.h>
#include <xci/widgets/FpsDisplay.h>
#include <xci/graphics/Window.h>
#include <xci/util/file.h>
#include <random>
#include <cstdlib>

using namespace xci::graphics;
using namespace xci::widgets;

int main()
{
    xci::util::chdir_to_share();

    Window& window = Window::default_window();
    window.create({800, 600}, "XCI UI demo");

    if (!Theme::load_default_theme())
        return EXIT_FAILURE;

    // Random color generator
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_int_distribution<int> runi(0, 255);
    auto random_color = [&](){ return Color(runi(re), runi(re), runi(re)); };

    Button button("Click me!");
    button.set_font_size(0.07);
    button.set_padding(0.05);
    button.on_click([&](View& view) {
        button.set_text_color(random_color());
        button.update(view);
    });

    Checkbox checkbox;
    checkbox.set_text("Checkbox");
    checkbox.set_position({0, 0.2});

    FpsDisplay fps_display;
    fps_display.set_position({-1.2f, -0.8f});

    Composite root;
    root.add(button);
    root.add(checkbox);
    root.add(fps_display);

    Bind bind(window, root);
    window.set_refresh_mode(RefreshMode::OnDemand);
    window.display();
    return EXIT_SUCCESS;
}
