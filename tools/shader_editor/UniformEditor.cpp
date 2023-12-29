// UniformEditor.cpp created on 2023-02-26 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "UniformEditor.h"

#include <xci/core/log.h>
#include <array>

namespace xci::shed {

using namespace xci::core;
using namespace xci::text;


UniformEditor::UniformEditor(Theme& theme)
    : Composite(theme), m_form(theme)
{
    add_child(m_form);
}


void UniformEditor::populate_form(const ShaderCompiler::ShaderResources& res)
{
    m_form.clear();
    m_form.add_label("Uniforms:").layout()
            .set_default_font_style(FontStyle::Bold)
            .set_default_outline_color(Color::Black())
            .set_default_outline_radius(1_px);

    static std::array color_shuffle = {Color::Green(), Color::White(), Color::Olive(), Color::Teal()};
    unsigned color_idx = 0;

    m_uniforms.clear();
    for (const auto& block : res.uniform_blocks) {
        for (const auto& member : block.members) {
            // set binding for last member only
            const unsigned binding = (&member == &block.members.back())? block.binding : ~0u;
            if (member.vec_size == 1) {
                m_uniforms.emplace_back(Uniform{0.0f, binding});
                float& value = get<float>(m_uniforms.back().value);
                auto [label, spinner] = m_form.add_input(member.name, value);
                label.layout()
                        .set_default_outline_color(Color::Black())
                        .set_default_outline_radius(1_px);
                spinner.on_change([this, &value](Spinner& o) {
                    value = o.value();
                    if (m_change_cb)
                        m_change_cb(*this);
                });
            } else if (member.vec_size == 4) {
                Color color = color_shuffle[color_idx++];
                if (color_idx >= color_shuffle.size())
                    color_idx = 0;
                m_uniforms.emplace_back(Uniform{color, binding});
                Color& value = get<Color>(m_uniforms.back().value);
                auto [label, color_picker] = m_form.add_input(member.name, value);
                label.layout()
                        .set_default_outline_color(Color::Black())
                        .set_default_outline_radius(1_px);
                color_picker.on_change([this, &value](ColorPicker& o) {
                    value = o.color();
                    if (m_change_cb)
                        m_change_cb(*this);
                });
            }
        }
    }
}


void UniformEditor::setup_uniforms(Primitives& prim)
{
    prim.clear_uniforms();

    std::vector<std::uint8_t> buffer;
    for (const auto& u : m_uniforms) {
        std::visit([&buffer](auto& v) {
            using T = std::decay_t<decltype(v)>;
            const size_t ofs = buffer.size();
            if constexpr (std::is_same_v<T, Color>) {
                const LinearColor data(v);
                buffer.resize(buffer.size() + sizeof(data));
                std::memcpy(buffer.data() + ofs, &data, sizeof(data));
            } else {
                buffer.resize(buffer.size() + sizeof(v));
                std::memcpy(buffer.data() + ofs, &v, sizeof(v));
            }
        }, u.value);
        if (u.binding != ~0u) {
            // flush buffer
            prim.set_uniform_data(u.binding, buffer.data(), buffer.size());
            buffer.clear();
        }
    }
    assert(buffer.empty());
}


} // namespace xci::shed
