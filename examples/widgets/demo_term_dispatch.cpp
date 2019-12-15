// demo_term_dispatch.cpp created on 2019-04-06, part of XCI toolkit
// Copyright 2019 Radek Brich
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
#include <xci/core/dispatch.h>
#include <xci/config.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <unistd.h>

using namespace xci::widgets;
using namespace xci::graphics;
using namespace xci::core;


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

int main()
{
    Logger::init();
    Vfs vfs;
    vfs.mount(XCI_SHARE_DIR);

    Renderer renderer {vfs};
    Window window {renderer};
    window.create({800, 600}, "XCI TextTerminal + Dispatch demo");

    Theme theme(renderer);
    if (!theme.load_default())
        return EXIT_FAILURE;

    const char* cmd = "while true ; do date ; sleep 1; done";

    TextTerminal terminal {theme};
    terminal.add_text(get_cwd() + "> ");
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

    Bind bind(window, terminal);
    window.set_refresh_mode(RefreshMode::OnDemand);
    window.set_view_mode(ViewOrigin::TopLeft, ViewScale::FixedScreenPixels);
    window.display();

    dispatch.terminate();
    return EXIT_SUCCESS;
}
