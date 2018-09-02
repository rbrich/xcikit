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
#include <xci/widgets/TextInput.h>
#include <xci/graphics/Window.h>
#include <xci/util/Vfs.h>
#include <xci/util/format.h>
#include <xci/config.h>
#include <random>
#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics;
using namespace xci::widgets;
using namespace xci::util;


class MousePosInfo: public Widget {
public:
    MousePosInfo() : m_text("Mouse: ", Theme::default_theme().font()) {
        m_text.set_color(Color(255, 150, 50));
    }

    void draw(View& view, State state) override {
        m_text.resize_draw(view, position());
    }

    void mouse_pos_event(View& view, const MousePosEvent& ev) override {
        m_text.set_fixed_string("Mouse: " +
                                format("({}, {})", ev.pos.x, ev.pos.y));
        view.refresh();
    }

private:
    Text m_text;
};


int main()
{
    Vfs::default_instance().mount_dir(XCI_SHARE_DIR);

    Window& window = Window::default_window();
    window.create({800, 600}, "XCI UI demo");

    if (!Theme::load_default_theme())
        return EXIT_FAILURE;

    Composite root;

    // Random color generator
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_int_distribution<int> runi(0, 255);
    auto random_color = [&](){ return Color(runi(re), runi(re), runi(re)); };

    for (auto i : {0,1,2,3,4}) {
        auto text_input = std::make_shared<TextInput>("input");
        text_input->set_position({-1.0f, -0.5f + i * 0.12f});
        root.add(text_input);
    }

    for (auto i : {0,1,2,3,4}) {
        auto button = std::make_shared<Button>(std::to_string(i+1) + ". click me!");
        button->set_position({-0.2f, -0.5f + i * 0.12f});
        auto* btn_self = button.get();
        button->on_click([btn_self, &random_color](View& view) {
            btn_self->set_text_color(random_color());
            btn_self->resize(view);
        });
        root.add(button);
    }

    for (auto i : {0,1,2,3,4}) {
        auto checkbox = std::make_shared<Checkbox>();
        checkbox->set_text("Checkbox " + std::to_string(i+1));
        checkbox->set_position({0.5f, -0.5f + i * 0.06f});
        root.add(checkbox);
    }

    auto mouse_pos_info = std::make_shared<MousePosInfo>();
    mouse_pos_info->set_position({-1.2f, 0.9f});
    root.add(mouse_pos_info);

    auto fps_display = std::make_shared<FpsDisplay>();
    fps_display->set_position({-1.2f, -0.8f});
    root.add(fps_display);

    Bind bind(window, root);
    window.set_refresh_mode(RefreshMode::OnDemand);
    window.display();
    return EXIT_SUCCESS;
}
