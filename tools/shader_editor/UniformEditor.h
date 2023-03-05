// UniformEditor.h created on 2023-02-26 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SHED_UNIFORM_EDITOR_H
#define XCI_SHED_UNIFORM_EDITOR_H

#include "ShaderCompiler.h"
#include <xci/widgets/Form.h>
#include <xci/widgets/Widget.h>
#include <xci/graphics/Primitives.h>
#include <deque>
#include <variant>
#include <functional>

namespace xci::shed {

using namespace xci::widgets;


class UniformEditor: public Composite {
public:
    explicit UniformEditor(Theme& theme);

    void populate_form(const ShaderCompiler::ShaderResources& res);

    void setup_uniforms(Primitives& prim);

    using ChangeCallback = std::function<void(UniformEditor&)>;
    void on_change(ChangeCallback cb) { m_change_cb = std::move(cb); }

private:
    Form m_form;

    struct Uniform {
        std::variant<Color, float> value;
        unsigned binding = ~0u;  // only set for last uniform in the block
    };
    std::deque<Uniform> m_uniforms;
    ChangeCallback m_change_cb;
};


}  // namespace xci::shed

#endif  // include guard
