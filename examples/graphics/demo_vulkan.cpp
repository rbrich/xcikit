// demo_vulkan.cpp created on 2019-10-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "common.h"

#include <xci/graphics/Primitives.h>
#include <xci/graphics/Shader.h>
#include <xci/graphics/Texture.h>
#include <xci/graphics/Shape.h>
#include <xci/core/Vfs.h>
#include <xci/config.h>

#include <cstdlib>

using namespace xci::graphics::unit_literals;


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

    // replace subregion of the texture
    for (uint32_t i = 0; i != 50*50; ++i) {
        pixels[i] = 128;
    }
    texture.write(pixels.data(), {100, 100, 50, 50});
    texture.update();
}


int main(int argc, const char* argv[])
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer(vfs);
    Window window {renderer};
    setup_window(window, "XCI Vulkan Demo", argv);

    Shader shader {renderer};
    shader.load_from_file(
            vfs.read_file("shaders/sprite_c.vert.spv").path(),
            vfs.read_file("shaders/sprite_c.frag.spv").path());

    // Low-level object for drawing primitives (in this case, quads)
    Primitives prim {renderer,
                     VertexFormat::V2c4t2, PrimitiveType::TriFans};

    Texture texture{renderer, ColorFormat::Grey};
    texture.create({256, 256});
    generate_checkerboard(texture);

    prim.set_shader(shader);
    prim.set_texture(1, texture);
    prim.set_blend(BlendFunc::AlphaBlend);

    // Colored polygon
    Primitives poly {renderer,
                     VertexFormat::V2c4, PrimitiveType::TriFans};
    Shader poly_shader {renderer};
    poly_shader.load_from_file(
            vfs.read_file("shaders/poly_c.vert.spv").path(),
            vfs.read_file("shaders/poly_c.frag.spv").path());
    poly.set_shader(poly_shader);
    poly.set_blend(BlendFunc::AlphaBlend);

    // Higher-level object which wraps Primitives and can draw different basic shapes
    // using specifically prepared internal shaders (in this case, it draws a rectangle)
    Shape shape {renderer};
    shape.set_fill_color(Color(30, 40, 50, 128));
    shape.set_outline_color(Color(180, 180, 0));
    shape.set_softness(1);
    shape.set_antialiasing(1);

    window.set_size_callback([&](View& view) {
        prim.clear();

        prim.begin_primitive();
        prim.add_vertex(view.vp_to_fb({-50_vp, -50_vp}), {1.0f, 0.0f, 0.0f}, 0, 0);
        prim.add_vertex(view.vp_to_fb({-50_vp, 0_vp}), {0.0f, 0.0f, 1.0f}, 0, 0);
        prim.add_vertex(view.vp_to_fb({0_vp, 0_vp}),  {1.0f, 0.0f, 1.0f}, 0, 0);
        prim.add_vertex(view.vp_to_fb({0_vp, -50_vp}), {1.0f, 1.0f, 0.0f}, 0, 0);
        prim.end_primitive();

        prim.begin_primitive();
        prim.add_vertex(view.vp_to_fb({-25_vp, -25_vp}), {1.0f, 0.0f, 0.0f}, 0, 0);
        prim.add_vertex(view.vp_to_fb({-25_vp, 25_vp}), {0.0f, 1.0f, 0.0f}, 0, 1.0);
        prim.add_vertex(view.vp_to_fb({25_vp, 25_vp}), {0.0f, 0.0f, 1.0f}, 1.0, 1.0);
        prim.add_vertex(view.vp_to_fb({25_vp, -25_vp}), {1.0f, 1.0f, 0.0f}, 1.0, 0);
        prim.end_primitive();

        prim.update();

        poly.clear();
        poly.begin_primitive();
        poly.add_vertex(view.vp_to_fb({20_vp, -40_vp}), {1.0f, 0.0f, 1.0f});
        poly.add_vertex(view.vp_to_fb({30_vp, 0_vp}), {0.0f, 1.0f, 0.0f});
        poly.add_vertex(view.vp_to_fb({60_vp, -10_vp}), {0.0f, 0.0f, 1.0f});
        poly.add_vertex(view.vp_to_fb({60_vp, -30_vp}), {0.0f, 1.0f, 1.0f});
        poly.add_vertex(view.vp_to_fb({40_vp, -50_vp}), {1.0f, 1.0f, 0.0f});
        poly.end_primitive();
        poly.update();

        shape.clear();
        shape.add_rectangle(view.vp_to_fb({-37.5_vp, -15_vp, 100_vp, 60_vp}), view.vp_to_fb(2.5_vp));
        shape.update();
    });

    window.set_draw_callback([&](View& view) {
        prim.draw(view);
        poly.draw(view);
        shape.draw(view, {0_fb, 0_fb});
    });

    window.set_refresh_mode(RefreshMode::OnDemand);
    window.display();
    return EXIT_SUCCESS;
}
