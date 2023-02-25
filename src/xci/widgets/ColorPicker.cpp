// ColorPicker.cpp created on 2023-02-24 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ColorPicker.h"

namespace xci::widgets {


ColorPicker::ColorPicker(Theme& theme, Color color)
        : Composite(theme),
          m_spinner_r(theme, color.r),
          m_spinner_g(theme, color.g),
          m_spinner_b(theme, color.b),
          m_spinner_a(theme, color.a),
          m_sample_box(theme.renderer()),
          m_color(color),
          m_decoration(theme.color(ColorId::Default))
{
    m_spinner_r.set_text_color(Color(255, 50, 0));
    m_spinner_g.set_text_color(Color(0, 192, 50));
    m_spinner_b.set_text_color(Color(50, 100, 255));
    m_spinner_a.set_text_color(theme.color(ColorId::Default));

    using std::ref;
    for (auto& spinner : {ref(m_spinner_r), ref(m_spinner_g), ref(m_spinner_b), ref(m_spinner_a)}) {
        spinner.get().set_format_cb([](float v) { return fmt::format("{:02X}", int(v)); });
        spinner.get().set_step(1, 16);
        spinner.get().set_bounds(0, 255);
        spinner.get().set_decoration_color(Color::Transparent(), Color::Transparent());
        spinner.get().set_outline_thickness(0_fb);
        spinner.get().set_padding({0.35_vp, 0.7_vp});
        spinner.get().on_change([this](View& view) { value_changed(view); });
        add_child(spinner.get());
    }
}


void ColorPicker::set_padding(VariSize padding)
{
    using std::ref;
    for (auto& spinner : {ref(m_spinner_r), ref(m_spinner_g), ref(m_spinner_b), ref(m_spinner_a)}) {
        spinner.get().set_padding({padding.x.mul(0.5), padding.y});
    }
}


void ColorPicker::resize(View& view)
{
    m_spinner_r.resize(view);
    auto rect = m_spinner_r.aabb();
    rect.x = 0;
    rect.y = 0;
    const auto spinner_w = rect.w;
    const auto sample_w = rect.h;
    const auto padding = view.to_fb(0.5_vp);
    rect.w = sample_w;
    m_sample_box.clear();
    m_sample_box.add_rectangle(rect, view.to_fb(m_outline_thickness));
    m_sample_box.update(m_color, m_decoration);

    rect.w += sample_w + padding + 4*spinner_w;
    set_size(rect.size());
    set_baseline(-rect.y);

    m_spinner_r.set_position({sample_w + padding + spinner_w*0, 0_fb});
    m_spinner_g.set_position({sample_w + padding + spinner_w*1, 0_fb});
    m_spinner_b.set_position({sample_w + padding + spinner_w*2, 0_fb});
    m_spinner_a.set_position({sample_w + padding + spinner_w*3, 0_fb});

    Composite::resize(view);
}


void ColorPicker::update(View& view, State state)
{
    if (state.focused) {
        m_decoration = theme().color(ColorId::Focus);
//    } else if (last_hover() == LastHover::Inside) {
//        m_decoration = theme().color(ColorId::Hover);
    } else {
        m_decoration = theme().color(ColorId::Default);
    }
    m_sample_box.update(m_color, m_decoration);
    Composite::update(view, state);
}


void ColorPicker::draw(View& view)
{
    m_sample_box.draw(view, position());
    Composite::draw(view);
}


void ColorPicker::value_changed(View& view)
{
    m_color = {
        uint8_t(m_spinner_r.value()),
        uint8_t(m_spinner_g.value()),
        uint8_t(m_spinner_b.value()),
        uint8_t(m_spinner_a.value()),
    };
    if (m_change_cb)
        m_change_cb(view);
    m_sample_box.update(m_color, m_decoration);
}


} // namespace xci::widgets
