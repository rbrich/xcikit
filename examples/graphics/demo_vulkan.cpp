// demo_vulkan.cpp created on 2019-10-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "common.h"

#include <xci/graphics/Primitives.h>
#include <xci/graphics/Shader.h>
#include <xci/graphics/Texture.h>
#include <xci/graphics/shape/Rectangle.h>
#include <xci/vfs/Vfs.h>
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


void update_poly(Primitives& poly, const View& view, float time_frac)
{
    poly.clear();
    poly.begin_primitive();
    float b1 = 20.0f;  // lower = thicker outline
    float b2 = 0.0f;
    constexpr int edges = 10;
    const float angle = 2 * std::acos(-1) / edges;
    poly.add_vertex(view.vp_to_fb({40_vp, -20_vp})).uvw(0.0f, 0.0f, b1);
    for (int i = 0; i <= edges; i++) {
        const float k = 0.5 * (1 + i % 2);
        auto v = Vec2{k*15_vp, k*15_vp}.rotate(-angle * (i + 2*time_frac));
        poly.add_vertex(view.vp_to_fb(Vec2{40_vp, -20_vp} + v)).uvw(b1, b2, 0.0f);
        std::swap(b1, b2);
    }
    poly.end_primitive();
    poly.update();
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
                     VertexFormat::V2t3, PrimitiveType::TriFans};
    Shader poly_shader {renderer};
    poly_shader.load_from_file(
            vfs.read_file("shaders/polygon.vert.spv").path(),
            vfs.read_file("shaders/polygon.frag.spv").path());
    poly.set_shader(poly_shader);
    poly.add_uniform(1, Color::Blue(), Color::Yellow());
    poly.add_uniform(2, 0.8f, 2);  // softness, antialiasing
    poly.set_blend(BlendFunc::AlphaBlend);

    // Higher-level object which wraps Primitives and can draw different basic shapes
    // using specifically prepared internal shaders (in this case, it draws a rectangle)
    Rectangle rect {renderer};

    float elapsed_acc = 0;

    window.set_size_callback([&](View& view) {
        prim.clear();

        prim.begin_primitive();
        prim.add_vertex(view.vp_to_fb({-50_vp, -50_vp})).color({1.0f, 0.0f, 0.0f}).uv(0, 0);
        prim.add_vertex(view.vp_to_fb({-50_vp, 0_vp})).color({0.0f, 0.0f, 1.0f}).uv(0, 0);
        prim.add_vertex(view.vp_to_fb({0_vp, 0_vp})).color({1.0f, 0.0f, 1.0f}).uv(0, 0);
        prim.add_vertex(view.vp_to_fb({0_vp, -50_vp})).color({1.0f, 1.0f, 0.0f}).uv(0, 0);
        prim.end_primitive();

        prim.begin_primitive();
        prim.add_vertex(view.vp_to_fb({-25_vp, -25_vp})).color({1.0f, 0.0f, 0.0f}).uv(0, 0);
        prim.add_vertex(view.vp_to_fb({-25_vp, 25_vp})).color({0.0f, 1.0f, 0.0f}).uv(0, 1.0);
        prim.add_vertex(view.vp_to_fb({25_vp, 25_vp})).color({0.0f, 0.0f, 1.0f}).uv(1.0, 1.0);
        prim.add_vertex(view.vp_to_fb({25_vp, -25_vp})).color({1.0f, 1.0f, 0.0f}).uv(1.0, 0);
        prim.end_primitive();

        prim.update();

        update_poly(poly, view, elapsed_acc);

        rect.clear();
        rect.add_rectangle(view.vp_to_fb({-10_vp, 10_vp, 60_vp, 40_vp}), view.vp_to_fb(2.5_vp));
        rect.update(Color(30, 40, 50, 128), Color(0, 180, 0), 0.5, 1);
    });

    window.set_update_callback([&](View& view, std::chrono::nanoseconds elapsed) {
        elapsed_acc += elapsed.count() / 1e9;
        elapsed_acc -= std::trunc(elapsed_acc);

        update_poly(poly, view, elapsed_acc);
    });

    window.set_draw_callback([&](View& view) {
        prim.draw(view);
        poly.draw(view);
        rect.draw(view, {0_fb, 0_fb});
    });

    window.set_refresh_mode(RefreshMode::Periodic);
    window.display();
    return EXIT_SUCCESS;
}
