// Window.cpp created on 2018-03-04, part of XCI toolkit

#include "WindowImpl.h"
#include "SpriteImpl.h"

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
    WindowFramework *window = framework.open_window();

    // XX panda text
    PT(TextNode) text;
    text = new TextNode("node name");
    text->set_text("Every day in every way I'm getting better and better.");
    PT(TextFont) cmr12=FontPool::load_font("fonts/Share_Tech_Mono/ShareTechMono-Regular.ttf");
    text->set_font(cmr12);
    text->set_text_color(0.0, 0.3, 0.1, 1);
    NodePath textNodePath = window->get_aspect_2d().attach_new_node(text);
    textNodePath.set_scale(0.07);
    textNodePath.set_pos(-1, 0, 0);
    // XX

    PT(GeomVertexData) vdata;
    vdata = new GeomVertexData("name", GeomVertexFormat::get_v3n3c4t2(), Geom::UH_static);
    vdata->set_num_rows(4);

    // texture
    auto image = PNMImage(256, 256);
    image.fill(0.3);
    PT(::Texture) tex = new ::Texture("test");
    tex->load(image);

    // quad
    GeomVertexWriter vertex, normal, color, texcoord;
    vertex = GeomVertexWriter(vdata, "vertex");
    normal = GeomVertexWriter(vdata, "normal");
    color = GeomVertexWriter(vdata, "color");
    texcoord = GeomVertexWriter(vdata, "texcoord");

    vertex.add_data3f(1, 0, 0);
    normal.add_data3f(0, 1, 0);
    color.add_data4f(0, 0, 1, 1);
    texcoord.add_data2f(1, 0);

    vertex.add_data3f(1, 0, 1);
    normal.add_data3f(0, 1, 0);
    color.add_data4f(0, 0, 1, 1);
    texcoord.add_data2f(1, 1);

    vertex.add_data3f(0, 0, 1);
    normal.add_data3f(0, 1, 0);
    color.add_data4f(0, 0, 1, 1);
    texcoord.add_data2f(0, 1);

    vertex.add_data3f(0, 0, 0);
    normal.add_data3f(0, 1, 0);
    color.add_data4f(0, 0, 1, 1);
    texcoord.add_data2f(0, 0);

    PT(GeomTrifans) prim;
    prim = new GeomTrifans(Geom::UH_static);
    prim->add_vertices(0, 1, 2, 3);
    prim->close_primitive();

    PT(Geom) geom;
    geom = new Geom(vdata);
    geom->add_primitive(prim);

    PT(GeomNode) node;
    node = new GeomNode("gnode");
    node->add_geom(geom);

    NodePath nodePath = window->get_aspect_2d().attach_new_node(node);
    nodePath.set_texture(tex);
}

void Window::display()
{
    PandaFramework& framework = m_impl->framework;
    framework.main_loop();
    framework.close_framework();
}

void Window::draw(const Sprite& sprite, const Vec2f& pos)
{

}


}} // namespace xci::graphics
