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

#include <xci/graphics/Window.h>
#include <xci/text/Text.h>
#include <xci/widgets/Button.h>

using namespace xci::graphics;
using namespace xci::text;
using namespace xci::widgets;

int main()
{
    Window window;
    window.create({800, 600}, "XCI widgets demo");

    FontFace face;
    if (!face.load_from_file("fonts/Share_Tech_Mono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;
    Font font;
    font.add_face(face);

    Button button("A button?", font);

    window.display([&](View& view) {
        button.draw(view, {0, 0});
    });
    return EXIT_SUCCESS;
}
