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
#include <xci/core/Vfs.h>
#include <xci/core/file.h>
#include <xci/core/format.h>
#include <xci/core/log.h>
#include <xci/config.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>

using namespace xci::widgets;
using namespace xci::graphics;
using namespace xci::core;

int main()
{
    Logger::init();
    Vfs vfs;
    vfs.mount(XCI_SHARE);

    Renderer renderer {vfs};
    Window window {renderer};
    window.create({800, 600}, "XCI TextTerminal demo");

    Theme theme(renderer);
    if (!theme.load_default())
        return EXIT_FAILURE;

    const char* cmd = "uname -a";

    TextTerminal terminal {theme};
    terminal.add_text(path::get_cwd() + "> ");
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
    terminal.add_text(path::get_cwd() + "> ");
    terminal.set_font_style(TextTerminal::FontStyle::Bold);
    terminal.add_text("rainbow\n");
    terminal.set_font_style(TextTerminal::FontStyle::Regular);

    // basic 16 colors
    terminal.set_fg(TextTerminal::Color4bit::BrightWhite);
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 8; col++) {
            terminal.set_bg(TextTerminal::Color8bit(row * 8 + col));
            terminal.add_text(format(" {:02x} ", row * 8 + col));
        }
        terminal.set_bg(TextTerminal::Color4bit::Black);
        terminal.set_fg(TextTerminal::Color4bit::Black);
        terminal.new_line();
    }
    terminal.new_line();

    // 216 color matrix (in 3 columns)
    terminal.set_fg(TextTerminal::Color4bit::BrightWhite);
    for (int row = 0; row < 12; row++) {
        for (int column = 0; column < 3; column++) {
            for (int i = 0; i < 6; i++) {
                auto idx = 16 + column * 72 + row * 6 + i;
                terminal.set_bg(TextTerminal::Color8bit(idx));
                terminal.add_text(format(" {:02x} ", idx));
            }
            terminal.set_bg(TextTerminal::Color4bit::Black);
            terminal.add_text(" ");
        }
        if (row == 5) {
            terminal.set_fg(TextTerminal::Color4bit::BrightWhite);
            terminal.new_line();
        }
        if (row == 2 || row == 8) {
            terminal.set_fg(TextTerminal::Color4bit::Black);
        }
        terminal.new_line();
    }
    terminal.new_line();

    // greyscale
    terminal.set_fg(TextTerminal::Color4bit::BrightWhite);
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 12; col++) {
            auto idx = 232 + row * 12 + col;
            terminal.set_bg(TextTerminal::Color8bit(idx));
            terminal.add_text(format(" {:02x} ", idx));
        }
        terminal.set_bg(TextTerminal::Color4bit::Black);
        terminal.set_fg(TextTerminal::Color4bit::Black);
        terminal.new_line();
    }

    terminal.reset_attrs();
    terminal.add_text(path::get_cwd() + "> ");
    terminal.set_font_style(TextTerminal::FontStyle::Bold);
    terminal.add_text("Příliš žluťoučký kůň úpěl ďábelské ódy.");

    terminal.set_position({50, 50});
    terminal.set_size({700, 500});

    window.set_key_callback([&](View& view, const KeyEvent& ev) {
        if (ev.action != Action::Press || ev.mod != ModKey::None())
        {
            switch (ev.key) {
                case Key::Up:
                    terminal.set_cursor_pos(terminal.cursor_pos() - Vec2u{0, 1});
                    break;
                case Key::Down:
                    terminal.set_cursor_pos(terminal.cursor_pos() + Vec2u{0, 1});
                    break;
                case Key::Left:
                    terminal.set_cursor_pos(terminal.cursor_pos() - Vec2u{1, 0});
                    break;
                case Key::Right:
                    terminal.set_cursor_pos(terminal.cursor_pos() + Vec2u{1, 0});
                    break;
                default:
                    break;
            }
        }
        view.refresh();
    });

    Bind bind(window, terminal);
    window.set_refresh_mode(RefreshMode::OnDemand);
    window.set_view_mode(ViewOrigin::TopLeft, ViewScale::FixedScreenPixels);
    window.display();
    return EXIT_SUCCESS;
}
