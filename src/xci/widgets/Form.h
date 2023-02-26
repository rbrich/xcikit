// Form.h created on 2018-06-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_WIDGETS_FORM_H
#define XCI_WIDGETS_FORM_H

#include <xci/widgets/Widget.h>
#include <xci/widgets/Label.h>
#include <xci/widgets/TextInput.h>
#include <xci/widgets/Checkbox.h>
#include <xci/widgets/Spinner.h>
#include <xci/widgets/ColorPicker.h>
#include <xci/core/container/ChunkedStack.h>

namespace xci::widgets {

using namespace graphics::unit_literals;


class Form: public Composite {
public:
    explicit Form(Theme& theme) : Composite(theme) {}

    void clear();

    // High-level interface
    Label& add_label(const std::string& label);
    void add_input(const std::string& label, std::string& text_input);
    void add_input(const std::string& label, bool& checkbox);
    void add_input(const std::string& label, float& spinner);
    void add_input(const std::string& label, Color& color);

    // Low-level interface
    enum class Hint {
        None,
        NextColumn,
        NextRow,
    };
    void add_hint(Hint hint) { add_hint(m_child.size(), hint); }
    void add_hint(size_t child_index, Hint hint);

    // override Composite
    void resize(View& view) override;

private:
    using Composite::clear_children;

    VariCoords m_margin = {1_vp, 1_vp};

    struct ChildHint {
        size_t child_index;
        Hint hint;

        bool operator<(const ChildHint& rhs) const {
            return child_index < rhs.child_index;
        }
    };
    std::vector<ChildHint> m_hint;
    core::ChunkedStack<Label> m_labels;
    core::ChunkedStack<TextInput> m_text_inputs;
    core::ChunkedStack<Checkbox> m_checkboxes;
    core::ChunkedStack<Spinner> m_spinners;
    core::ChunkedStack<ColorPicker> m_color_pickers;
};


} // namespace xci::widgets

#endif // XCI_WIDGETS_FORM_H
