// FpsDisplay.h created on 2018-04-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCIKIT_FPSDISPLAY_H
#define XCIKIT_FPSDISPLAY_H

#include <xci/widgets/Widget.h>
#include <xci/text/Text.h>
#include <xci/graphics/Primitives.h>
#include <xci/graphics/Texture.h>
#include <xci/core/FpsCounter.h>
#include <chrono>

namespace xci::widgets {


class FpsDisplay: public Widget {
public:
    explicit FpsDisplay(Theme& theme);

    void resize(View& view) override;
    void update(View& view, State state) override;
    void draw(View& view) override;

private:
    void create_sprite();
    void update_texture();

private:
    std::chrono::steady_clock::time_point m_prev_time = std::chrono::steady_clock::now();
    std::chrono::nanoseconds m_since_last_update {};
    core::FpsCounter m_fps;
    graphics::Primitives m_quad;
    graphics::Shader m_shader;
    graphics::Texture m_texture;
    text::Text m_text;
    bool m_frozen = false;
};


} // namespace xci::widgets

#endif // XCIKIT_FPSDISPLAY_H
