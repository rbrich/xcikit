// demo_panda.cpp - created by Radek Brich on 2018-03-07
//
// Shows how to integrate with Panda3d

#include <xci/graphics/panda/PandaWindow.h>

// Panda3D headers
#include <fontPool.h>

using namespace xci::graphics;

int main()
{
    PandaWindow window;
    window.create({800, 600}, "XCI Panda3D demo");

    // native Panda3D text
    PT(TextNode) panda_text;
    panda_text = new TextNode("node name");
    panda_text->set_text("Every day in every way I'm getting better and better.");
    PT(TextFont) cmr12 = FontPool::load_font("fonts/Share_Tech_Mono/ShareTechMono-Regular.ttf");
    panda_text->set_font(cmr12);
    panda_text->set_text_color(0.0, 1.0, 0.3, 1);
    NodePath textNodePath = window.panda_window()->get_aspect_2d().attach_new_node(panda_text);
    textNodePath.set_scale(0.07);
    textNodePath.set_pos(-1, 0, 0);

    window.display();
    return 0;
}
