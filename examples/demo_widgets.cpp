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
#include <xci/util/file.h>
#include <xci/util/format.h>
#include <cstdlib>

using namespace xci::widgets;
using namespace xci::text;
using namespace xci::graphics;
using namespace xci::util;

int main()
{
    xci::util::chdir_to_share();

    Window& window = Window::default_window();
    window.create({800, 600}, "XCI widgets demo");

    if (!Theme::load_default_theme())
        return EXIT_FAILURE;

    Button button_default("Default button");

    Button button_styled("Styled button");
    button_styled.set_font_size(0.07);
    button_styled.set_padding(0.05);
    button_styled.set_decoration_color(Color(10, 20, 100), Color(20, 50, 150));
    button_styled.set_text_color(Color(255, 255, 50));

    Icon checkbox;
    checkbox.set_icon(IconId::CheckBoxChecked);
    checkbox.set_text("checkbox");
    checkbox.set_size(0.08);
    checkbox.set_color(Color(200, 200, 200));

    window.display([&](View& view) {
        button_default.resize(view);
        button_default.draw(view, {0, -0.2f});
        button_styled.set_outline_thickness(1 * view.screen_ratio().y);
        button_styled.resize(view);
        button_styled.draw(view, {0, 0});

        checkbox.resize(view);
        checkbox.draw(view, {0, 0.4f});
    });
    return EXIT_SUCCESS;
}
