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
#include <xci/util/file.h>
#include <xci/util/format.h>
#include <random>
#include <cstdlib>

using namespace xci::graphics;
using namespace xci::widgets;
using namespace xci::util;


class MousePosInfo: public Widget {
public:
    MousePosInfo() : m_text("Mouse: ", Theme::default_theme().font()) {
        m_text.set_color(Color(255, 150, 50));
    }

    void resize(View& view) override { m_text.resize(view); }
    void draw(View& view, State state) override { m_text.draw(view, position()); }

    void handle(View& view, const MousePosEvent& ev) override {
        m_text.set_fixed_string("Mouse: " +
                                format("({}, {})", ev.pos.x, ev.pos.y));
        view.refresh();
    }

private:
    Text m_text;
};


int main()
{
    xci::util::chdir_to_share();

    Window& window = Window::default_window();
    window.create({800, 600}, "XCI form demo");

    if (!Theme::load_default_theme())
        return EXIT_FAILURE;

    Composite root;

    auto form = std::make_shared<Form>();
    form->set_position({-1.0f, -0.5f});
    root.add(form);

    std::string input_text = "2018-06-23";
    form->add_input("date", input_text);

    bool checkbox1 = false;
    form->add_input("checkbox1", checkbox1);

    bool checkbox2 = true;
    form->add_input("checkbox2", checkbox2);

    auto button = std::make_shared<Button>("submit");
    form->add_hint(Form::Hint::NextColumn);
    form->add(button);

    auto mouse_pos_info = std::make_shared<MousePosInfo>();
    mouse_pos_info->set_position({-1.2f, 0.9f});
    root.add(mouse_pos_info);

    auto output_text = std::make_shared<Label>();
    output_text->set_position({0.2f, -0.5f});
    output_text->text().set_color(Color(180, 100, 140));
    button->on_click([output_text, &input_text, &checkbox1, &checkbox2]
                     (View& view) {
        auto text = format("Submitted:\n\n"
                           "input_text = {}\n\n"
                           "checkbox1 = {}\n\n"
                           "checkbox2 = {}\n\n",
                           input_text, checkbox1, checkbox2);
        output_text->text().set_string(text);
        output_text->resize(view);
    });
    root.add(output_text);

    auto fps_display = std::make_shared<FpsDisplay>();
    fps_display->set_position({-1.2f, -0.8f});
    root.add(fps_display);

    Bind bind(window, root);
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
