// demo_oblong.cpp created on 2018-04-04, part of XCI toolkit
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
#include <xci/graphics/Shapes.h>
#include <xci/util/file.h>
#include <cstdlib>

using namespace xci::graphics;

int main()
{
    xci::util::chdir_to_share();

    Window window;
    window.create({800, 600}, "XCI rounded rectangle demo");

    Shapes rrect(Color(0, 20, 100, 128), Color(180, 180, 0), 2);
    rrect.add_rounded_rectangle({-1, -0.6f, 2, 1.2f}, 0.05, 0.01);

    window.display([&](View& view) {
        rrect.draw(view, {0, 0});
    });
    return EXIT_SUCCESS;
}
