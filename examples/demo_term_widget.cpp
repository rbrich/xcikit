// demo_term_widget.cpp created on 2018-07-19, part of XCI toolkit
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

#include <xci/widgets/TextTerminal.h>
#include <xci/graphics/Window.h>
#include <xci/util/file.h>
#include <xci/util/format.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>

using namespace xci::widgets;
using namespace xci::graphics;
using namespace xci::util;

int main()
{
    xci::util::chdir_to_share();

    Window& window = Window::default_window();
    window.create({800, 600}, "XCI TextTerminal demo");

    if (!Theme::load_default_theme())
        return EXIT_FAILURE;

    const char* cmd = "uname -a";

    TextTerminal terminal;
    terminal.add_text(get_cwd() + "> ");
    terminal.set_font_style(TextTerminal::FontStyle::Bold);
    terminal.add_text(std::string(cmd) + "\n");
    terminal.set_font_style(TextTerminal::FontStyle::Regular);
    terminal.set_fg(TextTerminal::Color4bit::BrightYellow);
    terminal.set_bg(TextTerminal::Color4bit::Blue);

    FILE* f = popen(cmd, "r");
    if (!f)
        return EXIT_FAILURE;
    std::string buf(256, 0);
    size_t nread;
    while ((nread = fread(&buf[0], 1, buf.size(), f)) > 0) {
        terminal.add_text(buf.substr(0, nread));
    }
    pclose(f);

    // Present some colors
    terminal.set_fg(TextTerminal::Color4bit::White);
    terminal.set_bg(TextTerminal::Color4bit::Black);
    terminal.add_text(get_cwd() + "> ");
    terminal.set_font_style(TextTerminal::FontStyle::Bold);
    terminal.add_text("color_palette\n");
    terminal.set_font_style(TextTerminal::FontStyle::Regular);

    // basic 16 colors
    for (int row = 0; row < 2; row++) {
        terminal.add_text("|");
        for (int col = 0; col < 8; col++) {
            terminal.set_fg(TextTerminal::Color8bit(row * 8 + col));
            terminal.add_text(format(" {:02x} ", row * 8 + col));
        }
        terminal.set_fg(TextTerminal::Color4bit::White);
        terminal.add_text("|");
        terminal.new_line();
    }
    terminal.new_line();

    // 216 color matrix (in 3 columns)
    for (int row = 0; row < 12; row++) {
        for (int column = 0; column < 3; column++) {
            if (column == 0)
                terminal.add_text("|");
            for (int i = 0; i < 6; i++) {
                auto idx = 16 + column * 72 + row * 6 + i;
                terminal.set_fg(TextTerminal::Color8bit(idx));
                terminal.add_text(format(" {:02x} ", idx));
            }
            terminal.set_fg(TextTerminal::Color4bit::White);
            terminal.add_text("|");
        }
        terminal.new_line();
    }
    terminal.new_line();

    // greyscale
    terminal.add_text("|");
    for (int i = 0; i < 24; i++) {
        auto idx = 232 + i;
        terminal.set_fg(TextTerminal::Color8bit(idx));
        terminal.add_text(format(" {:02x}", idx));
    }
    terminal.set_fg(TextTerminal::Color4bit::White);
    terminal.add_text(" |");

    // Make the terminal fullscreen
    window.set_size_callback([&](View& v) {
        auto s = v.scalable_size();
        terminal.set_position({-s * 0.5});
        terminal.set_size(s);
        terminal.bell();
    });

    Bind bind(window, terminal);
    window.set_refresh_mode(RefreshMode::OnDemand);
    window.display();
    return EXIT_SUCCESS;
}
