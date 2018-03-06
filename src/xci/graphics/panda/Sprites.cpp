// Sprites.cpp created on 2018-03-04, part of XCI toolkit

#include "SpritesImpl.h"
#include "ViewImpl.h"
#include "TextureImpl.h"

#include <geom.h>
#include <geomTrifans.h>
#include <geomVertexWriter.h>

namespace xci {
namespace graphics {


SpritesImpl::SpritesImpl(const Texture& texture)
        : vertex_data(new GeomVertexData("name",
                                         GeomVertexFormat::get_v3n3c4t2(),
                                         Geom::UH_static)),
          trifans(new GeomTrifans(Geom::UH_static)),
          texture(texture.impl().texture) {}


Sprites::Sprites(const Texture& texture) : m_impl(new SpritesImpl(texture)) {}
Sprites::~Sprites() { delete m_impl; }


void Sprites::add_sprite(const Vec2f& pos, const Color& color)
{
    add_sprite(pos, {0, 0,
                     m_impl->texture->get_x_size(),
                     m_impl->texture->get_y_size()},
               color);
}

void
Sprites::add_sprite(const Vec2f& pos, const Rect_u& texrect, const Color& color)
{
    auto& vdata = m_impl->vertex_data;
    vdata->set_num_rows(vdata->get_num_rows() + 4);

    // add a quad
    GeomVertexWriter wvertex, wcolor, wtexcoord;
    wvertex = GeomVertexWriter(vdata, "vertex");
    wcolor = GeomVertexWriter(vdata, "color");
    wtexcoord = GeomVertexWriter(vdata, "texcoord");

    wcolor.add_data4f(color.r, color.g, color.b, color.a);
    wcolor.add_data4f(color.r, color.g, color.b, color.a);
    wcolor.add_data4f(color.r, color.g, color.b, color.a);
    wcolor.add_data4f(color.r, color.g, color.b, color.a);

    wvertex.add_data3f(1, 0, 0);
    wtexcoord.add_data2f(1, 0);

    wvertex.add_data3f(1, 0, 1);
    wtexcoord.add_data2f(1, 1);

    wvertex.add_data3f(0, 0, 1);
    wtexcoord.add_data2f(0, 1);

    wvertex.add_data3f(0, 0, 0);
    wtexcoord.add_data2f(0, 0);

    auto& prim = m_impl->trifans;
    prim->add_vertices(0, 1, 2, 3);
    prim->close_primitive();
}

void Sprites::draw(View& view, const Vec2f& pos)
{
    PT(Geom) geom;
    geom = new Geom(m_impl->vertex_data);
    geom->add_primitive(m_impl->trifans);

    PT(GeomNode) node;
    node = new GeomNode("gnode");
    node->add_geom(geom);

    NodePath node_path = view.impl().root_node.attach_new_node(node);
    node_path.set_texture(m_impl->texture);
}


}} // namespace xci::graphics
