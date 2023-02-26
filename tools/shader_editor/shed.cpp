// shed.cpp created on 2023-02-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

/// Shader Editor (shed) tool
/// A tool for developing GLSL shaders.

#include "ShaderCompiler.h"
#include "UniformEditor.h"

#include <xci/widgets/Label.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/shape/Polygon.h>
#include <xci/graphics/vulkan/VulkanError.h>
#include <xci/core/ArgParser.h>
#include <xci/core/TermCtl.h>
#include <xci/core/dispatch.h>
#include <xci/core/log.h>

using namespace xci::core;
using namespace xci::core::argparser;
using namespace xci::text;
using namespace xci::shed;


static constexpr auto c_version = "0.1";


static bool reload_shader(ShaderCompiler& sc, Shader& shader, UniformEditor& unifed,
                          fs::path vert_path, fs::path frag_path)
{
    auto vert_spv = sc.compile_shader(ShaderStage::Vertex, vert_path);
    if (vert_spv.empty()) {
        log::error("Vertex shader failed to compile: {}", vert_path);
        return false;
    }

    auto frag_spv = sc.compile_shader(ShaderStage::Fragment, frag_path);
    if (frag_spv.empty()) {
        log::error("Fragment shader failed to compile: {}", frag_path);
        return false;
    }

    auto res = sc.reflect_shader(frag_spv);
    if (res)
        unifed.populate_form(res);

    return shader.load_from_memory(vert_spv, frag_spv);
}


int main(int argc, const char* argv[])
{
    fs::path vert_path, frag_path;
    uint32_t device_id = ~0u;
    bool show_version = false;
    bool log_debug = false;

    TermCtl& term = TermCtl::stdout_instance();

    ArgParser {
            Option("VERT", "Path to vertex shader to edit", vert_path),
            Option("FRAG", "Path to fragment shader to edit", frag_path),
            Option("-D, --device-id ID", "Select graphics device", device_id),
            Option("-v, --verbose", "Verbose logging", log_debug),
            Option("-V, --version", "Show version", show_version),
            Option("-h, --help", "Show help", show_help),
    } (argv);

    Logger::init(log_debug ? Logger::Level::Trace : Logger::Level::Warning);

    if (show_version) {
        term.print("{t:bold}shed{t:normal} {fg:*white}{}{t:normal}\n", c_version);
        return 0;
    }

    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};

    if (device_id != ~0u)
        window.renderer().set_device_id(device_id);

    try {
        window.create({1024, 768}, "XCI Shader Editor");
    } catch (const VulkanError& e) {
        log::error("VulkanError: {}", e.what());
        return EXIT_FAILURE;
    }

    Theme theme(renderer);
    if (!theme.load_default())
        return EXIT_FAILURE;

    FSDispatch watch;
    std::atomic_bool reload {false};
    auto reload_cb = [&reload](FSDispatch::Event ev) {
        if (ev == FSDispatch::Event::Create || ev == FSDispatch::Event::Modify)
            reload = true;
    };
    if (!watch.add_watch(vert_path, reload_cb) || !watch.add_watch(frag_path, reload_cb))
        return EXIT_FAILURE;

    Primitives prim {renderer,
                    VertexFormat::V2t2, PrimitiveType::TriFans};
    prim.set_blend(BlendFunc::AlphaBlend);

    UniformEditor unifed(theme);
    unifed.on_change([&prim](UniformEditor& o) {
        o.setup_uniforms(prim);
        prim.update();
    });

    Shader shader {renderer};
    ShaderCompiler compiler;
    if (!reload_shader(compiler, shader, unifed, vert_path, frag_path))
        return EXIT_FAILURE;
    prim.set_shader(shader);
    unifed.setup_uniforms(prim);

    Polygon poly {renderer};

    window.set_size_callback([&](View& view) {
        prim.clear();
        prim.begin_primitive();
        prim.add_vertex(view.vp_to_fb({-49_vp, -49_vp})).uv(-1, -1);
        prim.add_vertex(view.vp_to_fb({-49_vp, 49_vp})).uv(-1, 1);
        prim.add_vertex(view.vp_to_fb({49_vp, 49_vp})).uv(1, 1);
        prim.add_vertex(view.vp_to_fb({49_vp, -49_vp})).uv(1, -1);
        prim.end_primitive();
        prim.update();

        poly.clear();
        poly.add_polygon(view.vp_to_fb({0_vp, 0_vp}),
                         std::vector{
                                 view.vp_to_fb({-49_vp, -49_vp}),
                                 view.vp_to_fb({-49_vp, 49_vp}),
                                 view.vp_to_fb({49_vp, 49_vp}),
                                 view.vp_to_fb({49_vp, -49_vp}),
                                 view.vp_to_fb({-49_vp, -49_vp})},
                         1_fb);
        poly.update(Color::Transparent(), Color::Grey());
    });

    float elapsed_acc = 0;

    window.set_update_callback([&](View& view, std::chrono::nanoseconds elapsed) {
        elapsed_acc += elapsed.count() / 1e9;
        elapsed_acc -= std::trunc(elapsed_acc);

        if (reload) {
            reload = false;
            (void) reload_shader(compiler, shader, unifed, vert_path, frag_path);
            prim.set_shader(shader);
            unifed.setup_uniforms(prim);
            prim.update();
            unifed.resize(view);
        }

        // TODO: pass elapsed time to shader as uniform
    });

    window.set_draw_callback([&](View& view) {
        auto pop_offset = view.push_offset(view.to_fb(
                view.viewport_top_left({0.5 * view.viewport_size().x, 50_vp})));
        prim.draw(view);
        poly.draw(view);
    });

    window.set_key_callback([&window](View& v, const KeyEvent& ev) {
        if (ev.action != Action::Press)
            return;
        switch (ev.key) {
            case Key::Escape:
            case Key::Q:
                window.close();
                break;
            case Key::F11:
                window.toggle_fullscreen();
                break;
            default:
                break;
        }
    });

    window.set_refresh_mode(RefreshMode::Periodic);

    Bind bind(window, unifed);
    window.display();
    return EXIT_SUCCESS;
}
