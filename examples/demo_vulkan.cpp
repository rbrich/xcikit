// demo_opengl.cpp created on 2019-10-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/graphics/vulkan/VulkanRenderer.h>
#include <xci/graphics/vulkan/VulkanWindow.h>
#include <xci/graphics/vulkan/VulkanPrimitives.h>
#include <xci/graphics/vulkan/VulkanShader.h>
#include <xci/graphics/vulkan/VulkanTexture.h>
#include <xci/graphics/Shape.h>
#include <xci/core/Vfs.h>
#include <xci/config.h>

#include <cstdlib>
#include <cstring>

using namespace xci::graphics;
using namespace xci::core;


void generate_checkerboard(Texture& texture)
{
    std::vector<uint8_t> pixels(static_cast<VulkanTexture&>(texture).byte_size());
    auto size = texture.size();

    for (uint32_t y = 0; y != size.y; ++y)
        for (uint32_t x = 0; x != size.x; ++x) {
            const auto base = (y * size.x + x) * 4;
            const auto color = (x / 16 + y / 16) % 2 == 0
                    ? Color::Yellow()
                    : Color::Blue();
            std::memcpy(&pixels[base], &color, 4);
        }
    texture.update(pixels.data());
}


int main()
{
    Vfs vfs;
    vfs.mount(XCI_SHARE_DIR);

    VulkanRenderer renderer(vfs);
    VulkanWindow window {renderer};
    window.create({800, 600}, "XCI Vulkan Demo");

    VulkanShader shader {renderer.vk_device()};
    shader.load_from_file(
            vfs.read_file("shaders/sprite_c.vert.spv").path(),
            vfs.read_file("shaders/sprite_c.frag.spv").path());

    // Low-level object for drawing primitives (in this case, quads)
    VulkanPrimitives prim {renderer,
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

    auto texture = renderer.create_texture();
    texture->create({256, 256});
    generate_checkerboard(*texture);

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

    window.set_draw_callback([&](View& view) {
        prim.draw(view);
        shape.draw(view, {0, 0});
    });

    window.set_refresh_mode(RefreshMode::OnDemand);
    window.display();
    return EXIT_SUCCESS;
}
