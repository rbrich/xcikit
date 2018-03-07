// PandaWindow.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_PANDA_WINDOW_H
#define XCI_GRAPHICS_PANDA_WINDOW_H

#include <xci/graphics/Window.h>

#include <pandaFramework.h>

namespace xci {
namespace graphics {

class PandaWindow {
public:
    void create(const Vec2u& size, const std::string& title);
    void display(std::function<void(View& view)> draw_fn);

    View create_view();

    // access native handles
    WindowFramework* panda_window() { return m_window; }

private:
    PandaFramework m_framework;
    WindowFramework* m_window;
};

class Window::Impl : public PandaWindow {};

}} // namespace xci::graphics

#endif // XCI_GRAPHICS_PANDA_WINDOW_H
