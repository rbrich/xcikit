// demo_glyph_cluster.cpp created on 2019-12-16 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "graphics/common.h"

#include <xci/text/Font.h>
#include <xci/text/GlyphCluster.h>
#include <xci/graphics/Renderer.h>
#include <xci/graphics/Window.h>
#include <xci/vfs/Vfs.h>
#include <xci/core/FpsCounter.h>
#include <xci/config.h>
#include <ranges>
#include <random>
#include <cstdlib>

// sample data: g_dict
#include "sample.h"

using namespace xci::text;
using namespace xci::graphics;
using namespace xci::graphics::unit_literals;
using namespace xci::core;
using namespace std::chrono;
using namespace std::chrono_literals;


struct Word {
    Color color;
    ViewportCoords pos;
    size_t index;
    float speed;
    milliseconds age {};
    bool active = false;
};


static float lerp(float a, float b, float t) { return a + t * (b - a); }


int main(int argc, char* argv[])
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};

    setup_window(window, "XCI GlyphCluster demo", argv);

    Font font {renderer};
    if (!font.add_face(vfs, "fonts/Enriqueta/Enriqueta-Regular.ttf", 0))
        return EXIT_FAILURE;
    auto font_size = 25_px;

    GlyphCluster cluster {renderer, font};

    // Random generator
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_int_distribution<int> rbyte(0, 255);
    std::uniform_int_distribution<size_t> rword(0, std::size(g_dict) - 1);
    std::uniform_real_distribution<float> rpos(-0.5f, 0.5f);
    std::uniform_real_distribution<float> rspeed(0.01f, 0.10f);
    auto random_color = [&](){ return Color(rbyte(re), rbyte(re), rbyte(re)); };
    auto random_word = [&](){ return rword(re); };
    auto random_position = [&](){ return ViewportCoords(-0.5f, rpos(re)); };
    auto random_speed = [&](){ return rspeed(re); };

    std::chrono::nanoseconds timer {};

    std::array<Word, 100> words;

    FpsCounter fps;

    window.set_size_callback([&](View& view) {
        font.clear_cache();
        font.set_size(view.px_to_fb(font_size).as<unsigned int>());
    });

    window.set_update_callback([&](View& view, std::chrono::nanoseconds elapsed) {
        constexpr auto step = 50ms;
        constexpr auto alpha_cycle = 5s;
        timer += elapsed;
        if (timer >= step) {
            timer -= step;
            const auto it = std::ranges::find_if(words, [](Word& w) { return !w.active; });
            if (it != words.end()) {
                it->active = true;
                it->age = 0ms;
                it->color = random_color();
                it->pos = random_position() * view.viewport_size();
                it->index = random_word();
                it->speed = random_speed();
            }
        }

        view.finish_draw();
        cluster.clear();
        cluster.reserve(words.size() * 20);
        int active = 0;
        const auto elapsed_ms = duration_cast<milliseconds>(elapsed);
        for (auto& w : words) {
            if (!w.active || w.pos.x > view.viewport_size().x/2 || w.age + elapsed_ms > alpha_cycle) {
                w.active = false;
                continue;
            }
            ++active;
            w.age += elapsed_ms;
            const float t = 2.0f * w.age / alpha_cycle;
            w.color.a = (uint8_t)lerp(0.f, 255.f, t > 1 ? 2 - t : t);
            w.pos.x += ViewportUnits{w.speed * elapsed_ms.count()};
            cluster.set_color(w.color);
            cluster.set_pen(view.vp_to_fb(w.pos));
            cluster.add_string(view, g_dict[w.index]);
        }

        fps.tick(duration_cast<duration<float>>(elapsed).count());
        cluster.set_color(Color(100, 50, 255));
        cluster.set_pen(-0.5f * view.framebuffer_size() + view.px_to_fb({font_size, font_size}));
        cluster.add_string(view, fmt::format("{}fps ({:.2f}ms)",
                            fps.frame_rate(),
                            fps.avg_frame_time() * 1000));

        cluster.set_pen(-0.5f * view.framebuffer_size() + view.px_to_fb({10 * font_size, font_size}));
        cluster.add_string(view, fmt::format("[{} words, {} active]",
                        words.size(), active));

        cluster.recreate();
    });

    window.set_draw_callback([&](View& view) {
        cluster.draw(view, {});
    });

    window.set_refresh_mode(RefreshMode::Periodic);
    window.display();
    return EXIT_SUCCESS;
}
