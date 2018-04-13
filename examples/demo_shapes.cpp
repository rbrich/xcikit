// demo_shapes.cpp created on 2018-04-10, part of XCI toolkit
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
#include <xci/graphics/Shape.h>
#include <xci/util/file.h>
#include <cstdlib>

using namespace xci::graphics;

int main()
{
    xci::util::chdir_to_share();

    Window& window = Window::default_window();
    window.create({800, 600}, "XCI rectangles demo");

    Shape line(Color(0, 0, 40, 128), Color(180, 180, 0));
    line.set_softness(1.0);
    line.add_line_slice({-0.8f, -0.8f, 1.6f, 1.6f},
                        {0.0f, 0.0f}, {1.0f, 0.2f},
                        0.04);

    // using View::screen_ratio, we can set constant border width, in screen pixels
    Shape line_px(Color(40, 40, 0, 0), Color(255, 0, 0));

    window.display([&](View& view) {
        line.draw(view, {0, 0});

        auto pxr = view.screen_ratio().x;
        line_px.add_line_slice({-0.8f, -0.8f, 1.6f, 1.6f},
                              {0.0f, 0.0f}, {1.0f, 0.2f},
                              1*pxr);
        line_px.draw(view, {0, 0});
        line_px.clear();
    });
    return EXIT_SUCCESS;
}
