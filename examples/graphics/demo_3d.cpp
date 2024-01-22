// demo_3d.cpp created on 2023-12-29 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "common.h"

#include <xci/widgets/FpsDisplay.h>
#include <xci/graphics/Primitives.h>
#include <xci/graphics/shape/Rectangle.h>
#include <xci/math/transform.h>
#include <xci/core/log.h>
#include <xci/config.h>

#include <numbers>
#include <cstdlib>

using namespace xci;
using namespace xci::graphics::unit_literals;
using namespace xci::widgets;


static void create_unit_cube(Primitives& prim)
{
    prim.add_vertex({-0.5, 0.5, -0.5}).normal({0, 1, 0});
    prim.add_vertex({-0.5, 0.5, 0.5}).normal({0, 1, 0});
    prim.add_vertex({0.5, 0.5, 0.5}).normal({0, 1, 0});
    prim.add_vertex({0.5, 0.5, -0.5}).normal({0, 1, 0});
    prim.add_triangle_face({0, 1, 2});
    prim.add_triangle_face({0, 2, 3});

    prim.add_vertex({-0.5, 0.5, -0.5}).normal({-1, 0, 0});
    prim.add_vertex({-0.5, -0.5, -0.5}).normal({-1, 0, 0});
    prim.add_vertex({-0.5, -0.5, 0.5}).normal({-1, 0, 0});
    prim.add_vertex({-0.5, 0.5, 0.5}).normal({-1, 0, 0});
    prim.add_triangle_face({4, 5, 6});
    prim.add_triangle_face({4, 6, 7});

    prim.add_vertex({0.5, 0.5, 0.5}).normal({1, 0, 0});
    prim.add_vertex({0.5, -0.5, 0.5}).normal({1, 0, 0});
    prim.add_vertex({0.5, -0.5, -0.5}).normal({1, 0, 0});
    prim.add_vertex({0.5, 0.5, -0.5}).normal({1, 0, 0});
    prim.add_triangle_face({8, 9, 10});
    prim.add_triangle_face({8, 10, 11});

    prim.add_vertex({0.5, 0.5, -0.5}).normal({0, 0, -1});
    prim.add_vertex({0.5, -0.5, -0.5}).normal({0, 0, -1});
    prim.add_vertex({-0.5, -0.5, -0.5}).normal({0, 0, -1});
    prim.add_vertex({-0.5, 0.5, -0.5}).normal({0, 0, -1});
    prim.add_triangle_face({12, 13, 14});
    prim.add_triangle_face({12, 14, 15});

    prim.add_vertex({-0.5, 0.5, 0.5}).normal({0, 0, 1});
    prim.add_vertex({-0.5, -0.5, 0.5}).normal({0, 0, 1});
    prim.add_vertex({0.5, -0.5, 0.5}).normal({0, 0, 1});
    prim.add_vertex({0.5, 0.5, 0.5}).normal({0, 0, 1});
    prim.add_triangle_face({16, 17, 18});
    prim.add_triangle_face({16, 18, 19});

    prim.add_vertex({-0.5, -0.5, 0.5}).normal({0, -1, 0});
    prim.add_vertex({-0.5, -0.5, -0.5}).normal({0, -1, 0});
    prim.add_vertex({0.5, -0.5, -0.5}).normal({0, -1, 0});
    prim.add_vertex({0.5, -0.5, 0.5}).normal({0, -1, 0});
    prim.add_triangle_face({20, 21, 22});
    prim.add_triangle_face({20, 22, 23});
}


int main(int, const char* argv[])
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer(vfs);
    renderer.set_depth_buffering(true);

    // MSAA must be set before creating window. The max_sample_count() will be known after that,
    // but you can safely set more samples than supported, the value will be capped to max_sample_count.
    renderer.set_sample_count(8);

    Window window {renderer};
    setup_window(window, "XCI 3D Demo", argv);

    log::info("Multisampling: {}", renderer.sample_count());

    // Low-level object for drawing primitives (3D triangles)
    Primitives prim {renderer, VertexFormat::V3n3, PrimitiveType::TriList};
    create_unit_cube(prim);
    prim.set_depth_test(DepthTest::Less);
    prim.set_shader(renderer.get_shader("phong", "phong"));

    Theme theme(renderer);
    if (!theme.load_default())
        return EXIT_FAILURE;

    FpsDisplay fps_display(theme);
    fps_display.set_position({-60_vp, 45_vp});

    float elapsed_acc = 0;

    auto projection = Mat4f::identity();
    const auto view_matrix = look_at_view<float>({1.5, -3, 2}, {0,0,0}, {0,0,1});
    auto modelview_matrix = view_matrix * Mat4f::scale({2,2,2});
    auto normal_matrix = modelview_matrix.inverse_transpose();

    window.set_size_callback([&](View& view) {
        const auto size = view.framebuffer_size();
        projection = perspective_projection(1.2f, float(size.x.value / size.y.value), 0.1f, 10.f);

        const auto light_pos = view_matrix * Vec4f(1.0, -2.0, 4.0, 1.0);
        const Color light_ambient(0.0, 0.2, 0.0);
        const Color light_diffuse(0.5, 0.7, 0.5);
        const Color light_specular(0.6, 1.0, 0.6);
        const float light_quad_att = 0.1f;

        const Color mat_ambient(1.0, 1.0, 1.0);
        const Color mat_diffuse(1.0, 1.0, 1.0);
        const Color mat_specular(0.2, 0.2, 0.2);
        const float mat_shininess = 50.f;

        prim.set_dynamic_uniform(0).mat4(modelview_matrix).mat4(normal_matrix).mat4(projection);
        prim.set_uniform(1).vec4(light_pos)
                .color(light_ambient).color(light_diffuse).color(light_specular)
                .f(light_quad_att);
        prim.set_uniform(2).color(mat_ambient).color(mat_diffuse).color(mat_specular).f(mat_shininess);
        prim.update();

        fps_display.resize(view);
    });

    window.set_update_callback([&](View& view, std::chrono::nanoseconds elapsed) {
        elapsed_acc += elapsed.count() / 10e9;
        elapsed_acc -= std::trunc(elapsed_acc);

        const auto phi = elapsed_acc * 2 * std::numbers::pi_v<float>;
        modelview_matrix = view_matrix * Mat4f::scale({2,2,2}) * Mat4f::rot_z(cos(phi), sin(phi));
        normal_matrix = modelview_matrix.inverse_transpose();

        prim.set_dynamic_uniform(0).mat4(modelview_matrix).mat4(normal_matrix).mat4(projection);
        prim.update();

        fps_display.update(view, State{ elapsed });
    });

    window.set_draw_callback([&](View& view) {
        prim.draw(view, PrimitiveDrawFlags::FlipViewportY);
        fps_display.draw(view);
    });

    window.set_clear_color(Color(0.1, 0.0, 0.0));
    window.set_refresh_mode(RefreshMode::Periodic);
    renderer.set_present_mode(PresentMode::Immediate);
    window.display();
    return EXIT_SUCCESS;
}
