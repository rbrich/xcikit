// PandaSprites.cpp created on 2018-03-04, part of XCI toolkit

#include "PandaSprites.h"
#include "PandaView.h"
#include "PandaTexture.h"

#include <xci/util/log.h>
using namespace xci::util::log;

#include <geom.h>
#include <geomTrifans.h>
#include <geomVertexWriter.h>

// inline
#include <xci/graphics/Sprites.inl>

namespace xci {
namespace graphics {


PandaSprites::PandaSprites(const Texture& texture)
        : m_vertex_data(new GeomVertexData("Sprites",
                                         GeomVertexFormat::get_v3c4t2(),
                                         Geom::UH_static)),
          m_trifans(new GeomTrifans(Geom::UH_static)),
          m_texture(texture.impl().panda_texture()) {}



void PandaSprites::add_sprite(const Vec2f& pos, const Color& color)
{
    add_sprite(pos, {0, 0,
                     (unsigned) m_texture->get_x_size(),
                     (unsigned) m_texture->get_y_size()},
               color);
}

void
PandaSprites::add_sprite(const Vec2f& pos, const Rect_u& texrect, const Color& color)
{
    // add four vertices
    auto start_vertex = m_vertex_data->get_num_rows();
    m_vertex_data->reserve_num_rows(start_vertex + 4);

    GeomVertexWriter wvertex, wcolor, wtexcoord;
    wvertex = GeomVertexWriter(m_vertex_data, "vertex");
    wcolor = GeomVertexWriter(m_vertex_data, "color");
    wtexcoord = GeomVertexWriter(m_vertex_data, "texcoord");

    float r = color.red_f();
    float g = color.green_f();
    float b = color.blue_f();
    float a = color.alpha_f();
    wcolor.set_row(start_vertex);
    wcolor.add_data4f(r, g, b, a);
    wcolor.add_data4f(r, g, b, a);
    wcolor.add_data4f(r, g, b, a);
    wcolor.add_data4f(r, g, b, a);

    wvertex.set_row(start_vertex);
    wvertex.add_data3f(pos.x + texrect.w, 0, -pos.y);
    wvertex.add_data3f(pos.x + texrect.w, 0, -pos.y - texrect.h);
    wvertex.add_data3f(pos.x, 0,             -pos.y - texrect.h);
    wvertex.add_data3f(pos.x, 0,             -pos.y);

    auto tw = m_texture->get_x_size();
    auto th = m_texture->get_y_size();
    wtexcoord.set_row(start_vertex);
    wtexcoord.add_data2f((float)texrect.right() / tw, (float)texrect.top()    / th);
    wtexcoord.add_data2f((float)texrect.right() / tw, (float)texrect.bottom() / th);
    wtexcoord.add_data2f((float)texrect.left()  / tw, (float)texrect.bottom() / th);
    wtexcoord.add_data2f((float)texrect.left()  / tw, (float)texrect.top()    / th);

    // add a quad primitive (two triangles)
    m_trifans->add_consecutive_vertices(start_vertex, 4);
    bool res = m_trifans->close_primitive();
    assert(res && "close_primitive");
}

void PandaSprites::draw(View& view, const Vec2f& pos)
{
    PT(Geom) geom;
    geom = new Geom(m_vertex_data);
    geom->add_primitive(m_trifans);

    PT(GeomNode) node;
    node = new GeomNode("Sprites");
    node->add_geom(geom);

    NodePath node_path = view.impl().root_node.attach_new_node(node);
    node_path.set_texture(m_texture);
    node_path.set_transparency(TransparencyAttrib::M_alpha);
    node_path.set_pos(400+pos.x, 0, -300-pos.y);
}


}} // namespace xci::graphics
