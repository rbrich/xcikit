// PandaWindow.cpp created on 2018-03-04, part of XCI toolkit

#include "PandaWindow.h"
#include "PandaSprites.h"
#include "PandaView.h"

#include <pandaSystem.h>
#include <geomVertexFormat.h>
#include <geomTrifans.h>
#include <fontPool.h>
#include <pointerTo.h>

// inline
#include <xci/graphics/Window.inl>

namespace xci {
namespace graphics {


void PandaWindow::create(const Vec2u& size, const std::string& title)
{
    int argc = 0;
    char** argv = {nullptr};
    m_framework.open_framework(argc, argv);
    m_framework.set_window_title(title);
    m_framework.set_background_type(WindowFramework::BT_black);
    m_window = m_framework.open_window();
}


void PandaWindow::display(std::function<void(View& view)> draw_fn)
{
    View view = create_view();
    draw_fn(view);
    m_framework.main_loop();
    m_framework.close_framework();
}


View PandaWindow::create_view()
{
    View view;
    view.impl().root_node = m_window->get_pixel_2d();
    return view;
}


}} // namespace xci::graphics
