// MousePosInfo.h created on 2019-12-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_DEMO_MOUSEPOSINFO_H
#define XCI_DEMO_MOUSEPOSINFO_H

#include <xci/widgets/Widget.h>
#include <xci/text/Text.h>
#include <fmt/core.h>
#include <fmt/ostream.h>

namespace xci::demo {

using namespace xci::widgets;
using namespace xci::graphics;
using namespace xci::text;
using namespace xci::core;


class MousePosInfo: public Widget {
public:
    explicit MousePosInfo(Theme& theme)
            : Widget(theme),
              m_text(theme.font(), "Mouse: ")
    {
        m_text.set_color(Color(255, 150, 50));
    }

    void resize(View& view) override {
        Widget::resize(view);
        m_text.resize(view);
    }

    void update(View& view, State state) override {
        if (!m_pos_str.empty()) {
            m_text.set_fixed_string("Mouse: " + m_pos_str);
            m_text.update(view);
            view.refresh();
            m_pos_str.clear();
        }
    }

    void draw(View& view) override {
        m_text.draw(view, position());
    }

    void mouse_pos_event(View& view, const MousePosEvent& ev) override {
        m_pos_str = fmt::format("({}, {})", ev.pos.x, ev.pos.y);
    }

private:
    Text m_text;
    std::string m_pos_str;
};


} // namespace xci::demo

#endif // include guard
