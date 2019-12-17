// demo_glyph_cluster.cpp created on 2019-12-16 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/text/Font.h>
#include <xci/text/GlyphCluster.h>
#include <xci/graphics/Renderer.h>
#include <xci/graphics/Window.h>
#include <xci/core/Vfs.h>
#include <xci/core/string.h>
#include <xci/core/format.h>
#include <xci/core/FpsCounter.h>
#include <xci/config.h>
#include <random>
#include <cstdlib>

// sample data: g_dict
#include "sample.h"

using namespace xci::text;
using namespace xci::graphics;
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


float lerp(float a, float b, float t) { return a + t * (b - a); }


int main()
{
    Vfs vfs;
    vfs.mount(XCI_SHARE_DIR);

    Renderer renderer {vfs};
    Window window {renderer};
    window.create({800, 600}, "XCI GlyphCluster demo");

    Font font {renderer};
    if (!font.add_face(vfs, "fonts/Enriqueta/Enriqueta-Regular.ttf", 0))
        return EXIT_FAILURE;
    font.set_size(20);

    GlyphCluster cluster {renderer, font};

    // Random generator
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_int_distribution<int> rbyte(0, 255);
    std::uniform_int_distribution<size_t> rword(0, std::size(g_dict) - 1);
    std::uniform_real_distribution<float> rpos(-0.5f, 0.5f);
    std::uniform_real_distribution<float> rspeed(0.0001f, 0.0010f);
    auto random_color = [&](){ return Color(rbyte(re), rbyte(re), rbyte(re)); };
    auto random_word = [&](){ return rword(re); };
    auto random_position = [&](){ return ViewportCoords(-0.5f, rpos(re)); };
    auto random_speed = [&](){ return rspeed(re); };

    std::chrono::nanoseconds timer {};

    std::array<Word, 100> words;

    FpsCounter fps;

    window.set_update_callback([&](View& view, std::chrono::nanoseconds elapsed) {
        constexpr auto step = 200ms;
        constexpr auto alpha_cycle = 3000ms;
        timer += elapsed;
        if (timer >= step) {
            timer -= step;
            auto it = std::find_if(words.begin(), words.end(), [](Word& w) { return !w.active; });
            if (it != words.end()) {
                it->active = true;
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
            if (!w.active || w.pos.x > view.viewport_size().x/2) {
                w.active = false;
                continue;
            }
            ++active;
            w.age += elapsed_ms;
            if (w.age > alpha_cycle) w.age -= alpha_cycle;
            float t = 2.0f * w.age / alpha_cycle;
            w.color.a = (uint8_t)lerp(0.f, 255.f, t > 1 ? 2 - t : t);
            w.pos.x += ViewportUnits{w.speed * elapsed_ms.count()};
            cluster.set_color(w.color);
            cluster.set_pen(w.pos);
            cluster.add_string(view, g_dict[w.index]);
        }

        fps.tick(duration_cast<duration<float>>(elapsed).count());
        cluster.set_color(Color(100, 50, 255));
        cluster.set_pen({-1.0f, 0.95f});
        cluster.add_string(view,format("{}fps ({:.2f}ms)",
                            fps.frame_rate(),
                            fps.avg_frame_time() * 1000));

        cluster.set_pen({0.0f, 0.95f});
        cluster.add_string(view,format("[{} words, {} active]",
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
