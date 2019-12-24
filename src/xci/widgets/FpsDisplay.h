// FpsDisplay.h created on 2018-04-14, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef XCIKIT_FPSDISPLAY_H
#define XCIKIT_FPSDISPLAY_H

#include <xci/widgets/Widget.h>
#include <xci/text/Text.h>
#include <xci/graphics/Primitives.h>
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
    std::chrono::steady_clock::time_point m_prevtime = std::chrono::steady_clock::now();
    core::FpsCounter m_fps;
    graphics::Primitives m_quad;
    graphics::Shader& m_shader;
    graphics::Texture m_texture;
    text::Text m_text;
    bool m_frozen = false;
};


} // namespace xci::widgets

#endif // XCIKIT_FPSDISPLAY_H
