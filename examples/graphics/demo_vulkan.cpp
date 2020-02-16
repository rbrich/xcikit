// demo_vulkan.cpp created on 2019-10-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/graphics/Renderer.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/Primitives.h>
#include <xci/graphics/Shader.h>
#include <xci/graphics/Texture.h>
#include <xci/graphics/Shape.h>
#include <xci/core/Vfs.h>
#include <xci/config.h>

#include <cstdlib>

using namespace xci::graphics;
using namespace xci::core;


void generate_checkerboard(Texture& texture)
{
    std::vector<uint8_t> pixels(texture.byte_size());
    auto size = texture.size();

    // generate whole texture
    for (uint32_t y = 0; y != size.y; ++y)
        for (uint32_t x = 0; x != size.x; ++x) {
            pixels[y * size.x + x] =
                    (x / 16 + y / 16) % 2 == 0
                    ? 255 : 0;
        }
    texture.write(pixels.data());

    // replace sub-region of the texture
    for (uint32_t i = 0; i != 50*50; ++i) {
        pixels[i] = 128;
    }
    texture.write(pixels.data(), {100, 100, 50, 50});
    texture.update();
}


int main()
{
    Vfs vfs;
    vfs.mount(XCI_SHARE);

    Renderer renderer(vfs);
    Window window {renderer};
    window.create({800, 600}, "XCI Vulkan Demo");

    Shader shader {renderer};
    shader.load_from_file(
            vfs.read_file("shaders/sprite_c.vert.spv").path(),
            vfs.read_file("shaders/sprite_c.frag.spv").path());

    // Low-level object for drawing primitives (in this case, quads)
    Primitives prim {renderer,
                     VertexFormat::V2c4t2, PrimitiveType::TriFans};

    prim.begin_primitive();
    prim.add_vertex({-1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, 0, 0);
    prim.add_vertex({-1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 0, 0);
    prim.add_vertex({0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, 0, 0);
    prim.add_vertex({0.0f, -1.0f}, {1.0f, 1.0f, 0.0f}, 0, 0);
    prim.end_primitive();

    prim.begin_primitive();
    prim.add_vertex({-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, 0, 0);
    prim.add_vertex({-0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0, 1.0);
    prim.add_vertex({0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, 1.0, 1.0);
    prim.add_vertex({0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, 1.0, 0);
    prim.end_primitive();

    Texture texture{renderer};
    texture.create({256, 256});
    generate_checkerboard(texture);

    prim.set_shader(shader);
    prim.set_texture(1, texture);
    prim.set_blend(BlendFunc::AlphaBlend);

    // Higher-level object which wraps Primitives and can draw different basic shapes
    // using specifically prepared internal shaders (in this case, it draws a rectangle)
    Shape shape {renderer};
    shape.set_fill_color(Color(30, 40, 50, 128));
    shape.set_outline_color(Color(180, 180, 0));
    shape.set_softness(1);
    shape.set_antialiasing(1);
    shape.add_rectangle({-0.75, -0.3f, 2, 1.2f}, 0.05);

    prim.update();
    shape.update();

    window.set_draw_callback([&](View& view) {
        prim.draw(view);
        shape.draw(view, {0, 0});
    });

    window.set_refresh_mode(RefreshMode::OnDemand);
    window.display();
    return EXIT_SUCCESS;
}
