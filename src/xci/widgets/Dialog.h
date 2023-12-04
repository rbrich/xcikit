// Dialog.h created on 2023-12-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_WIDGETS_DIALOG_H
#define XCI_WIDGETS_DIALOG_H

#include <xci/widgets/Widget.h>
#include <xci/text/Text.h>
#include <vector>
#include <string>

namespace xci::widgets {


class Dialog: public Widget, public Padded, public text::TextMixin {
public:
    explicit Dialog(Theme& theme);

    struct SpanStyle {
        graphics::Color color;
        void apply(text::layout::Span& span) const;
    };
    using StyleIndex = unsigned;
    static constexpr StyleIndex default_normal_style = 0;
    static constexpr StyleIndex default_hover_style = 1;
    static constexpr StyleIndex default_focus_style = 2;
    static constexpr StyleIndex default_active_style = 3;
    StyleIndex add_style(SpanStyle&& style) { m_styles.push_back(std::move(style)); return m_styles.size() - 1; }
    SpanStyle& style(StyleIndex index) { return m_styles[index]; }

    struct Item {
        std::string span_name;
        uint16_t normal_style = default_normal_style;
        uint16_t hover_style = default_hover_style;
        uint16_t focus_style = default_focus_style;  // keyboard focus
        uint16_t active_style = default_active_style;  // clicked or key pressed
        graphics::Key key = graphics::Key::Unknown;  // hotkey to select the span
        graphics::Key alternative_key() const;  // for numeric keys, KeypadX is added automatically for NumX
    };
    /// Add dialog item manually. The text has to contain span of the specified name.
    Item& add_item(std::string span_name) {  return m_items.emplace_back(Item{std::move(span_name)}); }
    void clear_items() { m_items.clear(); m_selected_idx = none_selected; m_selection_type = SelectionType::None; }
    /// Automatically create dialog items for all spans in the text. Clears any existing items.
    void create_items_from_spans();
    /// Get item by `span_name`
    Item* get_item(std::string span_name);

    using ActivationCallback = std::function<void(View&, Item&)>;
    void on_activation(ActivationCallback cb) { m_activation_cb = std::move(cb); }

    // Event handlers
    void resize(View& view) override;
    void update(View& view, State state) override;
    void draw(View& view) override;
    bool key_event(View& view, const KeyEvent& ev) override;
    void mouse_pos_event(View& view, const MousePosEvent& ev) override;
    bool mouse_button_event(View& view, const MouseBtnEvent& ev) override;

private:
    void clear_selection();
    void handle_mouse_move(const FramebufferCoords& coords);
    bool handle_mouse_press(MouseButton button, const FramebufferCoords& coords);
    bool handle_mouse_release(View& view, MouseButton button, const FramebufferCoords& coords);

    std::vector<Item> m_items;
    std::vector<SpanStyle> m_styles {
            {Color(210, 190, 170)},  // normal
            {Color(255, 255, 255)},  // hover
            {Color(255, 255, 100)},  // focus
            {Color(255, 255, 130)},  // active
    };

    ActivationCallback m_activation_cb;

    static constexpr unsigned none_selected = (unsigned) -1;
    unsigned m_selected_idx = none_selected;

    enum class SelectionType: uint8_t { None, Hover, Click, KeyPress };
    SelectionType m_selection_type = SelectionType::None;
};


} // namespace xci::widgets

#endif  // XCI_WIDGETS_DIALOG_H
