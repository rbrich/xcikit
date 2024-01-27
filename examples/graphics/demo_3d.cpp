// demo_3d.cpp created on 2023-12-29 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "common.h"
#include "3d/Object.h"

#include <xci/widgets/FpsDisplay.h>
#include <xci/graphics/Primitives.h>
#include <xci/graphics/vulkan/Framebuffer.h>
#include <xci/graphics/shape/Rectangle.h>
#include <xci/math/transform.h>
#include <xci/core/log.h>
#include <xci/config.h>

#include <numbers>
#include <cstdlib>

using namespace xci;
using namespace xci::graphics::unit_literals;
using namespace xci::widgets;


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
    Object cube {renderer};
    cube.create_cube(1.0f);


    // Create offscreen framebuffer for mouse pick
    Attachments offscreen_attachments;
    offscreen_attachments.add_color_attachment({.format = VK_FORMAT_R32_UINT,
                                                .final_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT});
    offscreen_attachments.set_depth_bits(32);
    offscreen_attachments.create_renderpass(renderer.vk_device());
    offscreen_attachments.set_clear_color_value(0, { .uint32 = {0u} });
    Framebuffer offscreen(renderer);

//    Primitives sprite {renderer, VertexFormat::V2t2, PrimitiveType::TriFans};
//    sprite.set_shader(renderer.get_shader("sprite", "sprite"));
//    sprite.set_texture(2, offscreen.color_image_view(0, 0), renderer.get_sampler().vk());
//    sprite.set_blend(BlendFunc::AlphaBlend);

    FramebufferCoords mouse_pos = {0, 0};
    FramebufferCoords prev_mouse_pos = {0, 0};

    // out buffer
    VkBuffer out_buffer {};
    DeviceMemory out_memory(renderer);
    uint32_t* out_mapped = nullptr;
    uint32_t picked_object_id = 0;
    {
        const size_t byte_size = sizeof(int32_t) * 100;
        VkBufferCreateInfo buffer_ci = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = byte_size,
                .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        VK_TRY("vkCreateBuffer(out buffer)",
               vkCreateBuffer(renderer.vk_device(), &buffer_ci, nullptr, &out_buffer));
        VkMemoryRequirements mem_req;
        vkGetBufferMemoryRequirements(renderer.vk_device(), out_buffer, &mem_req);
        auto offset = out_memory.reserve(mem_req);
        assert(offset == 0);
        out_memory.allocate(
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        out_memory.bind_buffer(out_buffer, offset);
        out_mapped = (uint32_t*) out_memory.map(0, byte_size);
    }

    window.command_buffers().add_callback(CommandBuffers::Event::Init, nullptr,
                                          [&](CommandBuffer& cmd_buf, uint32_t image_index)
    {
        // Mouse pick offscreen renderpass - only if mouse moved
        if (mouse_pos == prev_mouse_pos)
            return;
        prev_mouse_pos = mouse_pos;

        const auto size = window.view().framebuffer_size();
        auto clear_values = offscreen_attachments.vk_clear_values();

        const VkRenderPassBeginInfo render_pass_info = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = offscreen_attachments.render_pass(),
                .framebuffer = offscreen.vk_framebuffer(0),
                .renderArea = {
                        .offset = {0, 0},
                        .extent = {size.x.as<uint32_t>(), size.y.as<uint32_t>()},
                },
                .clearValueCount = (uint32_t) clear_values.size(),
                .pClearValues = clear_values.data(),
        };
        vkCmdBeginRenderPass(cmd_buf.vk(), &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        cmd_buf.set_viewport(Vec2f(size), true);
        Rect_u mouse_region {mouse_pos.x.as<uint32_t>(), mouse_pos.y.as<uint32_t>(), 1u, 1u};
        cmd_buf.set_scissor(mouse_region);

        cube.prim().set_shader(renderer.get_shader("pick", "pick"));
        cube.draw(cmd_buf, offscreen_attachments, window.view());

        vkCmdEndRenderPass(cmd_buf.vk());

        cmd_buf.copy_image_to_buffer(offscreen.color_image(0, 0), mouse_region,
                                     out_buffer, 0, 1);
        cmd_buf.add_cleanup([&] {
            if (out_mapped != nullptr) {
                picked_object_id = *out_mapped;
            }
        });
    });

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
        offscreen.create(offscreen_attachments, {size.x.as<uint32_t>(), size.y.as<uint32_t>()}, 1);

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

        auto& prim = cube.prim();
        prim.set_dynamic_uniform(0).mat4(modelview_matrix).mat4(normal_matrix).mat4(projection);
        prim.set_uniform(1).vec4(light_pos)
                .color(light_ambient).color(light_diffuse).color(light_specular)
                .f(light_quad_att);
        prim.set_uniform(2).color(mat_ambient).color(mat_diffuse).color(mat_specular).f(mat_shininess);
        prim.update();

//        sprite.clear();
//        sprite.begin_primitive();
//        const auto rx = size.x / 4;
//        const auto ry = size.y / 4;
//        sprite.add_vertex({rx, 0_fb}).uv(0.0, 0.0);
//        sprite.add_vertex({rx, ry}).uv(0.0, 1.0);
//        sprite.add_vertex({rx*2, ry}).uv(1.0, 1.0);
//        sprite.add_vertex({rx*2, 0_fb}).uv(1.0, 0.0);
//        sprite.end_primitive();
//        sprite.update();

        fps_display.resize(view);
    });

    window.set_update_callback([&](View& view, std::chrono::nanoseconds elapsed) {
        elapsed_acc += elapsed.count() / 10e9;
        elapsed_acc -= std::trunc(elapsed_acc);

        const auto phi = elapsed_acc * 2 * std::numbers::pi_v<float>;
        modelview_matrix = view_matrix * Mat4f::scale({2,2,2}) * Mat4f::rot_z(cos(phi), sin(phi));
        normal_matrix = modelview_matrix.inverse_transpose();

        auto& prim = cube.prim();
        prim.set_dynamic_uniform(0).mat4(modelview_matrix).mat4(normal_matrix).mat4(projection);
        cube.update(42, picked_object_id);

        fps_display.update(view, State{ elapsed });
    });

    window.set_draw_callback([&](View& view) {
        auto& cmd = view.window()->command_buffer();
        cmd.set_viewport(Vec2f(view.framebuffer_size()), true);  // flipped Y
        cube.prim().set_shader(renderer.get_shader("phong", "phong"));
        cube.draw(view);
        cmd.set_viewport(Vec2f(view.framebuffer_size()), false);

        //sprite.draw(view);
        fps_display.draw(view);
    });

    window.set_mouse_position_callback([&mouse_pos](View& view, const MousePosEvent& ev) {
        mouse_pos = ev.pos + view.framebuffer_origin();
    });

    window.set_clear_color(Color(0.1, 0.0, 0.0));
    window.set_refresh_mode(RefreshMode::Periodic);
    renderer.set_present_mode(PresentMode::Immediate);
    window.display();

    offscreen.destroy();
    offscreen_attachments.destroy_renderpass(renderer.vk_device());

    out_mapped = nullptr;
    out_memory.unmap();
    out_memory.free();
    vkDestroyBuffer(renderer.vk_device(), out_buffer, nullptr);

    return EXIT_SUCCESS;
}
