// demo_form.cpp created on 2018-06-23, part of XCI toolkit
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
#include <xci/widgets/FpsDisplay.h>
#include <xci/widgets/Form.h>
#include <xci/graphics/Window.h>
#include <xci/widgets/Label.h>
#include <xci/core/Vfs.h>
#include <xci/core/format.h>
#include <xci/config.h>
#include <random>
#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics;
using namespace xci::widgets;
using namespace xci::core;


class MousePosInfo: public Widget {
public:
    explicit MousePosInfo(Theme& theme)
        : Widget(theme),
          m_text(theme.font(), "Mouse: ")
    {
        m_text.set_color(Color(255, 150, 50));
    }

    void resize(View& view) override {
        m_text.resize(view);
    }

    void update(View& view, State state) override {
        if (!m_pos_str.empty()) {
            m_text.set_fixed_string("Mouse: " + m_pos_str);
            m_text.update(view);
            view.refresh();
            m_pos_str.clear();
        }
    }

    void draw(View& view) override {
        m_text.draw(view, position());
    }

    void mouse_pos_event(View& view, const MousePosEvent& ev) override {
        m_pos_str = format("({}, {})", ev.pos.x, ev.pos.y);
    }

private:
    Text m_text;
    std::string m_pos_str;
};


int main()
{
    Vfs vfs;
    vfs.mount(XCI_SHARE_DIR);

    Renderer renderer {vfs};
    Window window {renderer};
    window.create({800, 600}, "XCI form demo");

    Theme theme(renderer);
    if (!theme.load_default())
        return EXIT_FAILURE;

    Composite root {theme};

    // Form #1
    Form form1 {theme};
    form1.set_position({-1.0f, -0.5f});
    root.add(form1);

    std::string input_text = "2018-06-23";
    form1.add_input("date", input_text);

    bool checkbox1 = false;
    form1.add_input("checkbox1", checkbox1);

    bool checkbox2 = true;
    form1.add_input("checkbox2", checkbox2);

    Button button(theme, "submit");
    form1.add_hint(Form::Hint::NextColumn);
    form1.add(button);

    // Form #1 output
    Label output_text(theme);
    output_text.set_position({0.2f, -0.5f});
    output_text.text().set_color(Color(180, 100, 140));
    button.on_click([&output_text, &input_text, &checkbox1, &checkbox2]
                     (View& view) {
        auto text = format("Submitted:\n\n"
                           "input_text = {}\n\n"
                           "checkbox1 = {}\n\n"
                           "checkbox2 = {}\n\n",
                           input_text, checkbox1, checkbox2);
        output_text.text().set_string(text);
        output_text.resize(view);
    });
    root.add(output_text);

    // Form #2
    Form form2(theme);
    form2.set_position({-1.0f, 0.2f});
    root.add(form2);

    std::string name = "Player1";
    form2.add_input("name", name);

    bool hardcore = false;
    form2.add_input("hardcore", hardcore);

    // Mouse pos
    MousePosInfo mouse_pos_info(theme);
    mouse_pos_info.set_position({-1.2f, 0.9f});
    root.add(mouse_pos_info);

    // FPS
    FpsDisplay fps_display(theme);
    fps_display.set_position({-1.2f, -0.8f});
    root.add(fps_display);

    window.set_refresh_mode(RefreshMode::OnDemand);
    //window.set_debug_flags(View::DebugFlags(View::Debug::LineBaseLine));

    window.set_key_callback([&root](View& v, const KeyEvent& e) {
        if (e.action == Action::Press && e.key == Key::D) {
            root.dump(std::cout);
            std::cout << std::endl;
        }
    });

    Bind bind(window, root);
    window.display();
    return EXIT_SUCCESS;
}
