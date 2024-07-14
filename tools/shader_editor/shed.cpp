// shed.cpp created on 2023-02-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

/// Shader Editor (shed) tool
/// A tool for developing GLSL shaders.

#include "ShaderCompiler.h"
#include "CoordEditor.h"
#include "UniformEditor.h"

#include <xci/widgets/Label.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/vulkan/VulkanError.h>
#include <xci/core/ArgParser.h>
#include <xci/core/TermCtl.h>
#include <xci/core/dispatch.h>
#include <xci/core/log.h>
#include <atomic>

using namespace xci::core;
using namespace xci::core::argparser;
using namespace xci::text;
using namespace xci::shed;


static constexpr auto c_version = "0.1";


static bool reload_vertex_shader(const ShaderCompiler& sc, ShaderModule& shader, fs::path path)
{
    auto vert_spv = sc.compile_shader(ShaderStage::Vertex, path);
    if (vert_spv.empty()) {
        log::error("Vertex shader failed to compile: {}", path);
        return false;
    }

    shader.create(vert_spv);
    return true;
}


static bool reload_fragment_shader(const ShaderCompiler& sc, ShaderModule& shader,
                                   UniformEditor& unifed, fs::path path)
{
    auto frag_spv = sc.compile_shader(ShaderStage::Fragment, path);
    if (frag_spv.empty()) {
        log::error("Fragment shader failed to compile: {}", path);
        return false;
    }

    auto res = sc.reflect_shader(frag_spv);
    if (res)
        unifed.populate_form(res);

    shader.create(frag_spv);
    return true;
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
        term.print("<bold>shed<normal> <*white>{}<normal>\n", c_version);
        return 0;
    }

    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};

    if (device_id != ~0u)
        window.renderer().set_device_id(device_id);

    if (!window.create({1024, 768}, "XCI Shader Editor"))
        return EXIT_FAILURE;

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

    ShaderModule vert_shader {renderer};
    ShaderModule frag_shader {renderer};
    ShaderCompiler compiler;
    if (!reload_vertex_shader(compiler, vert_shader, vert_path))
        return EXIT_FAILURE;
    if (!reload_fragment_shader(compiler, frag_shader, unifed, frag_path))
        return EXIT_FAILURE;
    prim.set_shader(Shader(vert_shader, frag_shader));
    unifed.setup_uniforms(prim);

    CoordEditor coord_editor(theme, prim);

    Label help(theme,
              "[F11] fullscreen\n"
              "[F1] hide help\n"
              "[F2] hide uniform editor\n"
              "[Esc] hide UI\n"
              "[f] hide frame\n"
              "[t] toggle triangle/quad\n"
              "[r] reset coords\n"
              "[q] quit\n");
    help.set_color(Color(200, 100, 50));
    help.set_outline_color(Color::Black());
    help.set_outline_radius(1_px);

    struct {
        bool all : 1 = false;
        bool help : 1 = false;
        bool frame : 1 = false;
        bool uniforms : 1 = false;
    } hide;

    window.set_size_callback([&](View& view) {
        auto tl = view.viewport_top_left({1_vp, 1_vp});
        unifed.set_position({-tl.x - 44_vp, tl.y});
        help.set_position({tl.x, tl.y});
    });

    float elapsed_acc = 0;

    window.set_update_callback([&](View& view, std::chrono::nanoseconds elapsed) {
        elapsed_acc += elapsed.count() / 1e9;
        elapsed_acc -= std::trunc(elapsed_acc);

        if (reload) {
            reload = false;
            (void) reload_vertex_shader(compiler, vert_shader, vert_path);
            (void) reload_fragment_shader(compiler, frag_shader, unifed, frag_path);
            prim.set_shader(Shader(vert_shader, frag_shader));
            unifed.setup_uniforms(prim);
            prim.update();
            unifed.resize(view);
        }

        // TODO: pass elapsed time to shader as uniform
    });

    window.set_draw_callback([&](View& view) {
        prim.draw(view);
    });

    window.set_key_callback([&](View& v, const KeyEvent& ev) {
        if (ev.action != Action::Press)
            return;
        switch (ev.key) {
            case Key::Escape:
                hide.all = ! hide.all;
                break;
            case Key::F:
                hide.frame = ! hide.frame;
                break;
            case Key::F1:
                hide.help = ! hide.help;
                break;
            case Key::F2:
                hide.uniforms = ! hide.uniforms;
                break;
            case Key::R:
                coord_editor.reset_coords();
                coord_editor.resize(v);
                break;
            case Key::T:
                coord_editor.toggle_triangle_quad();
                coord_editor.resize(v);
                break;
            case Key::Q:
                window.close();
                break;
            case Key::F11:
                window.toggle_fullscreen();
                break;
            default:
                break;
        }
        unifed.set_hidden(hide.all || hide.uniforms);
        coord_editor.set_hidden(hide.all || hide.frame);
        help.set_hidden(hide.all || hide.help);
    });

    window.set_refresh_mode(RefreshMode::OnEvent);

    Composite root {theme};
    root.add_child(coord_editor);
    root.add_child(unifed);
    root.add_child(help);

    Bind bind(window, root);
    window.display();
    return EXIT_SUCCESS;
}
