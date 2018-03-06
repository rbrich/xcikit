// Sprites.cpp created on 2018-03-04, part of XCI toolkit

#include "SpritesImpl.h"
#include "ViewImpl.h"
#include "TextureImpl.h"

#include <xci/util/log.h>
using namespace xci::util::log;

#include <geom.h>
#include <geomTrifans.h>
#include <geomVertexWriter.h>

namespace xci {
namespace graphics {


SpritesImpl::SpritesImpl(const Texture& texture)
        : vertex_data(new GeomVertexData("Sprites",
                                         GeomVertexFormat::get_v3c4t2(),
                                         Geom::UH_static)),
          trifans(new GeomTrifans(Geom::UH_static)),
          texture(texture.impl().texture) {}


Sprites::Sprites(const Texture& texture) : m_impl(new SpritesImpl(texture)) {}
Sprites::~Sprites() { delete m_impl; }


void Sprites::add_sprite(const Vec2f& pos, const Color& color)
{
    add_sprite(pos, {0, 0,
                     (unsigned) m_impl->texture->get_x_size(),
                     (unsigned) m_impl->texture->get_y_size()},
               color);
}

void
Sprites::add_sprite(const Vec2f& pos, const Rect_u& texrect, const Color& color)
{
    // add four vertices
    auto vdata = m_impl->vertex_data;
    auto start_vertex = vdata->get_num_rows();
    vdata->reserve_num_rows(start_vertex + 4);

    GeomVertexWriter wvertex, wcolor, wtexcoord;
    wvertex = GeomVertexWriter(vdata, "vertex");
    wcolor = GeomVertexWriter(vdata, "color");
    wtexcoord = GeomVertexWriter(vdata, "texcoord");

    wcolor.set_row(start_vertex);
    wcolor.add_data4f(color.r, color.g, color.b, color.a);
    wcolor.add_data4f(color.r, color.g, color.b, color.a);
    wcolor.add_data4f(color.r, color.g, color.b, color.a);
    wcolor.add_data4f(color.r, color.g, color.b, color.a);

    wvertex.set_row(start_vertex);
    wvertex.add_data3f(pos.x + texrect.w, 0, -pos.y);
    wvertex.add_data3f(pos.x + texrect.w, 0, -pos.y - texrect.h);
    wvertex.add_data3f(pos.x, 0,             -pos.y - texrect.h);
    wvertex.add_data3f(pos.x, 0,             -pos.y);

    auto tw = m_impl->texture->get_x_size();
    auto th = m_impl->texture->get_y_size();
    wtexcoord.set_row(start_vertex);
    wtexcoord.add_data2f((float)texrect.right() / tw, (float)texrect.top()    / th);
    wtexcoord.add_data2f((float)texrect.right() / tw, (float)texrect.bottom() / th);
    wtexcoord.add_data2f((float)texrect.left()  / tw, (float)texrect.bottom() / th);
    wtexcoord.add_data2f((float)texrect.left()  / tw, (float)texrect.top()    / th);

    // add a quad primitive (two triangles)
    auto prim = m_impl->trifans;
    prim->add_consecutive_vertices(start_vertex, 4);
    bool res = prim->close_primitive();
    assert(res && "close_primitive");
}

void Sprites::draw(View& view, const Vec2f& pos)
{
    PT(Geom) geom;
    geom = new Geom(m_impl->vertex_data);
    geom->add_primitive(m_impl->trifans);

    PT(GeomNode) node;
    node = new GeomNode("Sprites");
    node->add_geom(geom);

    NodePath node_path = view.impl().root_node.attach_new_node(node);
    node_path.set_texture(m_impl->texture);
    node_path.set_transparency(TransparencyAttrib::M_alpha);
    node_path.set_pos(400+pos.x, 0, -300-pos.y);
}


}} // namespace xci::graphics
