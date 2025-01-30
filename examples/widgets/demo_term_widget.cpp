// demo_term_widget.cpp created on 2018-07-19 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "graphics/common.h"

#include <xci/widgets/TextTerminal.h>
#include <xci/vfs/Vfs.h>
#include <xci/core/file.h>
#include <xci/core/log.h>
#include <xci/config.h>

#include <fmt/format.h>
#include <cstdlib>
#include <cstdio>

using namespace xci;
using namespace xci::widgets;
using namespace xci::graphics::unit_literals;
using fmt::format;

int main(int argc, const char* argv[])
{
    Logger::init();
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};
    setup_window(window, "XCI TextTerminal demo", argv);

    Theme theme(window);
    if (!theme.load_default())
        return EXIT_FAILURE;

    const char* cmd = "uname -a";
    auto prompt = fs::current_path().string() + "> ";

    TextTerminal terminal {theme};
    terminal.set_size_in_cells({100, 50});
    terminal.set_font_size(18_px);
    terminal.add_text(prompt);
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
    while ((nread = fread(buf.data(), 1, buf.size(), f)) > 0) {
        terminal.add_text(buf.substr(0, nread));
    }
    pclose(f);

    // Present some colors
    terminal.set_fg(TextTerminal::Color4bit::White);
    terminal.set_bg(TextTerminal::Color4bit::Black);
    terminal.add_text(prompt);
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
    terminal.add_text(prompt);
    terminal.set_font_style(TextTerminal::FontStyle::Bold);
    terminal.add_text("test_unicode\n");
    terminal.set_font_style(TextTerminal::FontStyle::Regular);
    terminal.set_fg(TextTerminal::Color8bit{214});
    terminal.add_text("🐎 Příliš žluťoučký kůň úpěl ďábelské ódy. 🐎\n");

    terminal.reset_attrs();
    terminal.add_text(prompt);
    terminal.set_font_style(TextTerminal::FontStyle::Bold);
    terminal.add_text("test_attrs\n");

    terminal.set_font_style(TextTerminal::FontStyle::Light);
    terminal.add_text("Light\n");
    terminal.set_font_style(TextTerminal::FontStyle::LightItalic);
    terminal.add_text("LightItalic\n");
    terminal.set_font_style(TextTerminal::FontStyle::Regular);
    terminal.add_text("Regular\n");
    terminal.set_font_style(TextTerminal::FontStyle::Italic);
    terminal.add_text("Italic\n");
    terminal.set_font_style(TextTerminal::FontStyle::Bold);
    terminal.add_text("Bold\n");
    terminal.set_font_style(TextTerminal::FontStyle::BoldItalic);
    terminal.add_text("BoldItalic\n");

    terminal.set_position({5_px, 0_px});

    // Make the terminal fullscreen
    window.set_size_callback([&](View& view) {
        auto vs = view.screen_size();
        vs.x -= 10_px;
        terminal.set_size(vs);
        view.refresh();
    });

    window.set_key_callback([&](View& view, const KeyEvent& ev) {
        if (ev.action != Action::Press || ev.mod != ModKey::None())
        {
            switch (ev.key) {
                case Key::Escape:
                    window.close();
                    break;
                case Key::F:
                    window.toggle_fullscreen();
                    break;
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
    window.set_view_origin(ViewOrigin::TopLeft);
    window.display();
    return EXIT_SUCCESS;
}
