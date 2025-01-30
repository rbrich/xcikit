// demo_3d.cpp created on 2023-12-29 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "common.h"
#include "3d/Object.h"

#include <xci/widgets/FpsDisplay.h>
#include <xci/widgets/Label.h>
#include <xci/widgets/Spinner.h>
#include <xci/widgets/Checkbox.h>
#include <xci/graphics/Primitives.h>
#include <xci/graphics/vulkan/Framebuffer.h>
#include <xci/graphics/vulkan/Buffer.h>
#include <xci/graphics/shape/Rectangle.h>
#include <xci/math/transform.h>
#include <xci/core/log.h>
#include <xci/config.h>

#include <numbers>
#include <cstdlib>

using namespace xci;
using namespace xci::graphics::unit_literals;
using namespace xci::widgets;


class Offscreen {
public:
    explicit Offscreen(Renderer& renderer) : m_framebuffer(renderer) {}
    ~Offscreen() {
        m_framebuffer.destroy();
        m_attachments.destroy_renderpass(m_framebuffer.renderer().vk_device());
    }

    void create(Renderer& renderer, const Attachments::ColorAttachment& color_attachment,
                VkClearColorValue clear_color)
    {
        m_attachments.add_color_attachment(color_attachment);
        m_attachments.set_depth_bits(32);
        m_attachments.create_renderpass(renderer.vk_device());
        m_attachments.set_clear_color_value(0, clear_color);
    }

    void resize(FramebufferSize size) {
        m_framebuffer.create(m_attachments, {size.x.as<uint32_t>(), size.y.as<uint32_t>()}, 1);
    }

    void begin_renderpass(CommandBuffer& cmd_buf, FramebufferSize size,
                          Rect_u scissor = {0, 0, INT32_MAX, INT32_MAX})
    {
        auto clear_values = m_attachments.vk_clear_values();

        const VkRenderPassBeginInfo render_pass_info = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = m_attachments.render_pass(),
                .framebuffer = m_framebuffer.vk_framebuffer(0),
                .renderArea = {
                        .offset = {0, 0},
                        .extent = {size.x.as<uint32_t>(), size.y.as<uint32_t>()},
                },
                .clearValueCount = (uint32_t) clear_values.size(),
                .pClearValues = clear_values.data(),
        };
        vkCmdBeginRenderPass(cmd_buf.vk(), &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        cmd_buf.set_viewport(Vec2f(size), true);
        cmd_buf.set_scissor(scissor);
    }

    Attachments& attachments() { return m_attachments; }
    Framebuffer& framebuffer() { return m_framebuffer; }

private:
    Attachments m_attachments;
    Framebuffer m_framebuffer;
};


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

    Theme theme(window);
    if (!theme.load_default())
        return EXIT_FAILURE;

    Composite root {theme};

    // Low-level object for drawing primitives (3D triangles)
    Object cube {renderer};
    cube.create_cube(1.0f);

    // Offscreen framebuffer for mouse pick
    Offscreen offscreen_pick(renderer);
    offscreen_pick.create(renderer,
                          {.format = VK_FORMAT_R32_UINT,
                           .final_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT},
                          { .uint32 = {0u} });

    // Offscreen framebuffer for glow effect on mouse pick
    // We draw the highlighted object into this buffer, then blur it and blit into main framebuffer.
    Offscreen offscreen_glow(renderer);
    offscreen_glow.create(renderer,
                           {.format = VK_FORMAT_B8G8R8A8_UNORM,
                            .final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            .usage = VK_IMAGE_USAGE_SAMPLED_BIT},
                           { .float32 = {0.0f, 0.0f, 0.0f, 0.0f} });

    Primitives glow {renderer, VertexFormat::V2t2, PrimitiveType::TriFans};
    struct GlowPushConstants {
        Vec2f center = {0.5f, 0.5f};
        float radius = 0.15f;
        float resolution = 100.0f;
    };
    GlowPushConstants glow_constants;
    glow.reserve_push_constants(sizeof(GlowPushConstants));
    glow.set_shader(renderer.get_shader("sprite", "blur_radial"));
    glow.set_blend(BlendFunc::AlphaBlend);

    Label l_glow_spinner(theme, "Glow size:");
    l_glow_spinner.set_position({-13_vp, 45_vp});
    l_glow_spinner.set_color(Color::Cyan());
    root.add_child(l_glow_spinner);
    Spinner glow_spinner(theme, 0.5f);
    glow_spinner.set_position({0_vp, 45_vp});
    glow_spinner.set_value(glow_constants.radius);
    glow_spinner.set_bounds(0.0f, 0.5f);
    glow_spinner.on_change([&glow_constants, &glow](Spinner& spinner) {
        glow_constants.radius = spinner.value();
        glow.set_push_constants_data(&glow_constants, sizeof(GlowPushConstants));
    });
    root.add_child(glow_spinner);

    Label l_vsync(theme, "vsync");
    l_vsync.set_position({-30_vp, 45_vp});
    l_vsync.set_color(Color::Cyan());
    root.add_child(l_vsync);
    Checkbox vsync_checkbox(theme);
    vsync_checkbox.set_checked(true);
    vsync_checkbox.on_change([&renderer](const Checkbox& c) {
        renderer.set_present_mode(c.checked() ? PresentMode::Fifo : PresentMode::Immediate);
    });
    vsync_checkbox.set_position({-33_vp, 46_vp});
    root.add_child(vsync_checkbox);

    FramebufferCoords mouse_pos = {0, 0};

    // Mouse pick object_id buffer
    graphics::Buffer out_buffer;
    DeviceMemory out_memory(renderer);
    uint32_t* out_mapped = nullptr;
    uint32_t picked_object_id = 0;
    {
        const size_t byte_size = sizeof(int32_t);
        const auto offset = out_buffer.create(renderer.vk_device(), out_memory,
                                              byte_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        assert(offset == 0);
        out_memory.allocate(
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        out_memory.bind_buffer(out_buffer.vk(), offset);
        out_mapped = (uint32_t*) out_memory.map(0, byte_size);
    }

    window.command_buffers().add_callback(CommandBuffers::Event::Init, nullptr,
                                          [&](CommandBuffer& cmd_buf, uint32_t image_index)
    {
        // Mouse pick offscreen renderpass
        const auto size = window.view().framebuffer_size();
        Rect_u mouse_region {mouse_pos.x.as<uint32_t>(), mouse_pos.y.as<uint32_t>(), 1u, 1u};
        offscreen_pick.begin_renderpass(cmd_buf, size, mouse_region);

        cube.prim().set_shader(renderer.get_shader("pick", "pick"));
        cube.draw(cmd_buf, offscreen_pick.attachments(), window.view());

        vkCmdEndRenderPass(cmd_buf.vk());

        cmd_buf.copy_image_to_buffer(offscreen_pick.framebuffer().color_image(0, 0),
                                     mouse_region,
                                     out_buffer.vk(), 0, 1);
        cmd_buf.add_cleanup([&] {
            if (out_mapped != nullptr) {
                picked_object_id = *out_mapped;
            }
        });

        // Glow effect offscreen renderpass
        if (picked_object_id == 42) {
            offscreen_glow.begin_renderpass(cmd_buf, size);
            cube.prim().set_shader(renderer.get_shader("phong", "phong"));
            cube.draw(cmd_buf, offscreen_glow.attachments(), window.view());
            vkCmdEndRenderPass(cmd_buf.vk());
        }
    });

    FpsDisplay fps_display(theme);
    fps_display.set_position({-60_vp, 45_vp});
    root.add_child(fps_display);

    float elapsed_acc = 0;

    auto projection = Mat4f::identity();
    const auto view_matrix = look_at_view<float>({1.5, -3, 2}, {0,0,0}, {0,0,1});
    auto modelview_matrix = view_matrix * Mat4f::scale({2,2,2});
    auto normal_matrix = modelview_matrix.inverse_transpose();

    window.set_size_callback([&](View& view) {
        mouse_pos = {0, 0};

        const auto size = view.framebuffer_size();
        offscreen_pick.resize(size);
        offscreen_glow.resize(size);

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

        glow.clear();
        glow.begin_primitive();
        const auto rx = size.x / 2;
        const auto ry = size.y / 2;
        glow.add_vertex({-rx, -ry}).uv(0.0, 0.0);
        glow.add_vertex({-rx, ry}).uv(0.0, 1.0);
        glow.add_vertex({rx, ry}).uv(1.0, 1.0);
        glow.add_vertex({rx, -ry}).uv(1.0, 0.0);
        glow.end_primitive();
        glow.set_texture(2, offscreen_glow.framebuffer().color_image_view(0, 0),
                         renderer.get_sampler().vk());
        glow_constants.resolution = 0.15f * std::max(size.x.value, size.y.value);
        glow.set_push_constants_data(&glow_constants, sizeof(GlowPushConstants));
        glow.update();
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
    });

    window.set_draw_callback([&](View& view) {
        if (picked_object_id == 42)
            glow.draw(view);

        auto& cmd = view.window()->command_buffer();
        cmd.set_viewport(Vec2f(view.framebuffer_size()), true);  // flipped Y
        cube.prim().set_shader(renderer.get_shader("phong", "phong"));
        cube.draw(view);
        cmd.set_viewport(Vec2f(view.framebuffer_size()), false);
    });

    window.set_mouse_position_callback([&mouse_pos](View& view, const MousePosEvent& ev) {
        mouse_pos = ev.pos + view.framebuffer_origin();
    });

    Bind bind(window, root);
    window.set_clear_color(Color(0.1, 0.0, 0.0));
    window.set_refresh_mode(RefreshMode::Periodic);
    renderer.set_present_mode(PresentMode::Fifo);
    window.display();

    out_mapped = nullptr;
    out_memory.unmap();
    out_memory.free();
    out_buffer.destroy(renderer.vk_device());

    return EXIT_SUCCESS;
}
