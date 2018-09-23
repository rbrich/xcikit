// demo_panda.cpp created on 2018-03-07, part of XCI toolkit
//
// Shows how to integrate with Panda3d

#include <xci/text/FontLibrary.h>
#include <xci/text/FontFace.h>
#include <xci/text/Font.h>
#include <xci/text/Text.h>
#include <xci/graphics/panda/PandaView.h>
#include <xci/core/file.h>

// Panda3D headers
#include <pandaFramework.h>
#include <fontPool.h>

// Caution, this may cause name collisions
using namespace xci::graphics;
using namespace xci::text;

const char *font_path = "fonts/ShareTechMono/ShareTechMono-Regular.ttf";

int main(int argc, char **argv)
{
    xci::core::chdir_to_share();

    // Panda3D init
    PandaFramework framework;
    framework.open_framework(argc, argv);
    framework.set_window_title("XCI Panda3D Demo");
    framework.set_background_type(WindowFramework::BT_black);
    WindowFramework* window = framework.open_window();

    // Panda3D text
    PT(TextNode) panda_text;
    panda_text = new TextNode("node name");
    panda_text->set_text("Every day in every way I'm getting better and better.");
    PT(TextFont) cmr12 = FontPool::load_font(font_path);
    panda_text->set_font(cmr12);
    panda_text->set_text_color(0.0, 1.0, 0.3, 1);
    NodePath textNodePath = window->get_aspect_2d().attach_new_node(panda_text);
    textNodePath.set_scale(0.07);
    textNodePath.set_pos(-1, 0, 0);

    // XCI text
    FontFace face;
    if (!face.load_from_file(font_path, 0))
        return EXIT_FAILURE;

    Font font;
    font.add_face(face);

    Text text;
    text.set_string("Every day in every way I'm getting better and better.");
    text.set_font(font);
    text.set_size(20);
    text.set_color(Color(0.0, 1.0, 0.3));

    View view = create_view_from_nodepath(window->get_pixel_2d());
    text.draw(view, {-200, -50});

    // Panda3D main
    framework.main_loop();
    framework.close_framework();
    return 0;
}
