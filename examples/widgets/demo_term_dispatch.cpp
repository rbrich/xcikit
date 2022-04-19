// demo_term_dispatch.cpp created on 2019-04-06 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "graphics/common.h"

#include <xci/widgets/TextTerminal.h>
#include <xci/core/Vfs.h>
#include <xci/core/file.h>
#include <xci/core/log.h>
#include <xci/core/dispatch.h>
#include <xci/config.h>
#include <xci/compat/unistd.h>
#include <cstdlib>
#include <cstdio>
#include <mutex>

using namespace xci::widgets;


class SharedBuffer {
public:
    void lock() { m_mutex.lock(); }
    void unlock() { m_mutex.unlock(); }

    char* data() { return m_buffer; }
    constexpr size_t size() const { return sizeof(m_buffer); }

    void set_pending(size_t pending) { m_pending = pending; }
    size_t pending() const { return m_pending; }

private:
    std::mutex m_mutex;
    char m_buffer[256];
    size_t m_pending = 0;
};

int main(int argc, const char* argv[])
{
    Logger::init();
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};
    setup_window(window, "XCI TextTerminal + Dispatch demo", argv);

    Theme theme(renderer);
    if (!theme.load_default())
        return EXIT_FAILURE;

    const char* cmd = "while true ; do date ; sleep 1; done";

    TextTerminal terminal {theme};
    terminal.add_text(fs::current_path().string() + "> ");
    terminal.set_font_style(TextTerminal::FontStyle::Bold);
    terminal.add_text(std::string(cmd) + "\n");
    terminal.set_font_style(TextTerminal::FontStyle::Regular);

    FILE* f = popen(cmd, "r");
    if (!f)
        return EXIT_FAILURE;
    setbuf(f, nullptr);

    Dispatch dispatch;
    SharedBuffer buffer;
    IOWatch io_watch(dispatch.loop(), fileno(f), IOWatch::Read,
         [f, &window, &buffer](int fd, IOWatch::Event event){
             switch (event) {
                 case IOWatch::Event::Read: {
                     std::lock_guard<SharedBuffer> guard(buffer);
                     if (buffer.pending() > 0)
                         break;
                     size_t nread;
                     if ((nread = ::read(fileno(f), buffer.data(), buffer.size())) > 0) {
                         buffer.set_pending(nread);
                         window.wakeup();
                     }
                     break;
                 }
                 case IOWatch::Event::Error:
                     //window.close();
                     break;
                 default: break;
             }
         });

    window.set_update_callback([&terminal, &buffer](View& v, std::chrono::nanoseconds){
        std::lock_guard<SharedBuffer> guard(buffer);
        if (buffer.pending() > 0) {
            terminal.add_text({buffer.data(), buffer.pending()});
            terminal.bell();
            buffer.set_pending(0);
            v.refresh();
        }
    });

    // Make the terminal fullscreen
    window.set_size_callback([&](View& v) {
        auto vs = v.viewport_size();
        terminal.set_size(vs);
    });

    window.set_key_callback([&](View& view, KeyEvent ev) {
        if (ev.action != Action::Press)
            return;
        switch (ev.key) {
            case Key::Escape:
                window.close();
                break;
            case Key::F11:
                window.toggle_fullscreen();
                break;
            default:
                break;
        }
    });

    Bind bind(window, terminal);
    window.set_refresh_mode(RefreshMode::Periodic);  // FIXME: bell() doesn't work with OnDemand
    window.set_view_mode(ViewOrigin::TopLeft);
    window.display();

    dispatch.terminate();
    return EXIT_SUCCESS;
}
