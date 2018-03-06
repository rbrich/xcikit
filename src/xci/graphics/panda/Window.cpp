// Window.cpp created on 2018-03-04, part of XCI toolkit

#include "WindowImpl.h"
#include "SpritesImpl.h"
#include "ViewImpl.h"

#include <pandaSystem.h>
#include <geomVertexFormat.h>
#include <geomTrifans.h>
#include <fontPool.h>
#include <pointerTo.h>

namespace xci {
namespace graphics {

Window::Window() : m_impl(new WindowImpl) {}
Window::~Window() { delete m_impl; }

void Window::create(const Vec2u& size, const std::string& title)
{
    PandaFramework& framework = m_impl->framework;
    int argc = 0;
    char** argv = {nullptr};
    framework.open_framework(argc, argv);
    framework.set_window_title(title);
    framework.set_background_type(WindowFramework::BT_black);
    m_impl->window = framework.open_window();

    /*
    // panda text
    PT(TextNode) text;
    text = new TextNode("node name");
    text->set_text("Every day in every way I'm getting better and better.");
    PT(TextFont) cmr12=FontPool::load_font("fonts/Share_Tech_Mono/ShareTechMono-Regular.ttf");
    text->set_font(cmr12);
    text->set_text_color(0.0, 0.3, 0.1, 1);
    NodePath textNodePath = m_impl->window->get_aspect_2d().attach_new_node(text);
    textNodePath.set_scale(0.07);
    textNodePath.set_pos(-1, 0, 0);
    */
}

void Window::display()
{
    PandaFramework& framework = m_impl->framework;
    framework.main_loop();
    framework.close_framework();
}

View Window::create_view()
{
    View view;
    view.impl().root_node = m_impl->window->get_pixel_2d();
    return view;
}


}} // namespace xci::graphics
