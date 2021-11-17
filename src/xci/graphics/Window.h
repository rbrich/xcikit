// Window.h created on 2018-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_WINDOW_H
#define XCI_GRAPHICS_WINDOW_H

#include "View.h"
#include <xci/graphics/vulkan/CommandBuffers.h>
#include <xci/graphics/Color.h>
#include <xci/core/geometry.h>
#include <xci/core/mixin.h>

#include <vulkan/vulkan.h>

#include <string>
#include <memory>
#include <functional>
#include <utility>
#include <chrono>

struct GLFWwindow;

namespace xci::graphics {

using xci::core::Vec2u;
using xci::core::Vec2f;
using xci::core::Vec2i;

// Key names come from GLFW, with only minor changes
enum class Key: uint8_t {
    Unknown = 0,
    F1 = 1,
    F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Escape,
    Enter,
    Backspace,
    Tab,
    Insert, Delete,
    Home, End,
    PageUp, PageDown,
    Left, Right, Up, Down,
    CapsLock, ScrollLock, NumLock,
    PrintScreen,
    Pause,

    // The following key codes correspond to ASCII
    Space = 32,
    Apostrophe  =   39, // '
    Comma = 44,     // ,
    Minus = 45,     // -
    Period = 46,    // .
    Slash = 47,     // /
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Semicolon = 59, // ;
    Equal = 61,     // =
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftBracket = 91,   // [
    Backslash = 92,     // \ //
    RightBracket = 93,  // ]
    GraveAccent = 96,   // `

    // International keys (non-US layout)
    World1 = 101,
    World2 = 102,

    // Numeric keypad
    Keypad0 = 128, Keypad1, Keypad2, Keypad3, Keypad4, Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
    KeypadAdd, KeypadSubtract, KeypadMultiply, KeypadDivide, KeypadDecimalPoint, KeypadEnter,

    // Modifier keys
    LeftShift, RightShift,
    LeftControl, RightControl,
    LeftAlt, RightAlt,
    LeftSuper, RightSuper,  // Command or Windows key
    Menu,                   // Windows menu key
};
static_assert((int)Key::Z == 90, "ascii letters");


struct ModKey {
    bool shift : 1;
    bool ctrl : 1;
    bool alt : 1;

    static constexpr ModKey None() { return {false, false, false}; }
    static constexpr ModKey Shift() { return {true, false, false}; }
    static constexpr ModKey Ctrl() { return {false, true, false}; }
    static constexpr ModKey Alt() { return {false, false, true}; }
    static constexpr ModKey ShiftCtrl() { return {true, true, false}; }
    static constexpr ModKey ShiftAlt() { return {true, false, true}; }
    static constexpr ModKey CtrlAlt() { return {false, true, true}; }
    static constexpr ModKey ShiftCtrlAlt() { return {true, true, true}; }

    bool operator== (ModKey other) const
        { return shift == other.shift && ctrl == other.ctrl && alt == other.alt;}
    bool operator!= (ModKey other) const { return !(*this == other); }
};


enum class Action {
    Release = 0,
    Press = 1,
    Repeat = 2,
};

struct KeyEvent {
    Key key;
    ModKey mod;
    Action action;
};


struct CharEvent {
    char32_t code_point;
};


enum class MouseButton { Left = 0, Right = 1, Middle = 2 };

struct MousePosEvent {
    ViewportCoords pos;
};

struct MouseBtnEvent {
    MouseButton button;
    Action action;
    ViewportCoords pos;
};


struct ScrollEvent {
    Vec2f offset;
};


enum class RefreshMode {
    OnDemand,   // got refresh event from system or called View::refresh()
    OnEvent,    // got any event from system
    Periodic,   // continuous refresh
};


class Renderer;


class Window: private core::NonCopyable {
public:
    explicit Window(Renderer& renderer) : m_renderer(renderer), m_command_buffers(renderer) {}
    ~Window();

    Renderer& renderer();

    // Create the window.
    void create(const Vec2u& size, const std::string& title);

    // Run main loop. Doesn't exit until the window is closed.
    void display();

    // When in OnDemand or OnEvent refresh mode, call this to wake up
    // event loop. Put custom handler into UpdateCallback.
    // (thread-safe)
    void wakeup() const;

    // Stop the main loop and close the window.
    // (thread-safe)
    void close() const;

    // Toggle fullscreen / windowed mode
    // The window starts in windowed mode.
    void toggle_fullscreen();

    void set_clipboard_string(const std::string& s) const;
    std::string get_clipboard_string() const;

    using UpdateCallback = std::function<void(View&, std::chrono::nanoseconds elapsed)>;
    using SizeCallback = std::function<void(View&)>;
    using DrawCallback = std::function<void(View&)>;
    using KeyCallback = std::function<void(View&, const KeyEvent&)>;
    using CharCallback = std::function<void(View&, const CharEvent&)>;
    using MousePosCallback = std::function<void(View&, const MousePosEvent&)>;
    using MouseBtnCallback = std::function<void(View&, const MouseBtnEvent&)>;
    using ScrollCallback = std::function<void(View&, const ScrollEvent&)>;

    // The original callback is replaced. To cascade callbacks,
    // you have to get and wrap original callback manually.
    void set_update_callback(UpdateCallback update_cb) { m_update_cb = std::move(update_cb); }
    void set_size_callback(SizeCallback size_cb) { m_size_cb = std::move(size_cb); }
    void set_draw_callback(DrawCallback draw_cb);
    void set_key_callback(KeyCallback key_cb) { m_key_cb = std::move(key_cb); }
    void set_char_callback(CharCallback char_cb) { m_char_cb = std::move(char_cb); }
    void set_mouse_position_callback(MousePosCallback mpos_cb);
    void set_mouse_button_callback(MouseBtnCallback mbtn_cb);
    void set_scroll_callback(ScrollCallback scroll_cb);

    UpdateCallback update_callback() { return m_update_cb; }
    SizeCallback size_callback() { return m_size_cb; }
    DrawCallback draw_callback() { return m_draw_cb; }
    KeyCallback key_callback() { return m_key_cb; }
    CharCallback char_callback() { return m_char_cb; }
    MousePosCallback mouse_position_callback() { return m_mpos_cb; }
    MouseBtnCallback mouse_button_callback() { return m_mbtn_cb; }
    ScrollCallback scroll_callback() { return m_scroll_cb; }

    /// Color used to clear the framebuffer after swapping. Default: black
    void set_clear_color(Color color) { m_clear_color = color; }

    // Refresh mode:
    // - OnDemand is energy-saving mode, good for normal GUI applications (forms etc.)
    // - OnEvent is similar, but does not require explicit calls to View::refresh()
    // - Periodic is good for games (continuous animations)
    void set_refresh_mode(RefreshMode mode)  { m_refresh_mode = mode; }

    /// Set refresh timeout. This is useful for OnDemand/OnEvent modes,
    /// where no update events are generated unless an event occurs.
    /// When configured, the update is called at most after `timetout`,
    /// but possibly sooner if something else happens.
    /// \param timeout      The timeout in usecs. Zero disables the timeout.
    /// \param periodic     False = one-shot (timeout is cleared after next update).
    ///                     True = periodic (no clear).
    void set_refresh_timeout(std::chrono::microseconds timeout, bool periodic);

    /// Select kind of viewport units to be used throughout the program
    /// for all placing and sizes of elements in view.
    /// \param origin       The position of (0,0) coordinates. Default is Center.
    /// \param scale        The scale of the units. Default is ScalingWithAspectCorrection.
    void set_view_mode(ViewOrigin origin, ViewScale scale);

    void set_debug_flags(View::DebugFlags flags);

    /// Wait for asynchronous draw commands to finish.
    /// This needs to be called before recreating objects that are being drawn.
    void finish_draw();

    // GLFW handles
    GLFWwindow* glfw_window() const { return m_window; }

    // Vulkan - current command buffer
    VkCommandBuffer vk_command_buffer() const { return m_command_buffers[m_current_cmd_buf]; }
    uint32_t command_buffer_index() const { return m_current_cmd_buf; }
    void add_command_buffer_resource(const ResourcePtr& resource) { m_command_buffers.add_resource(m_current_cmd_buf, resource); }

private:
    void setup_view();
    void create_command_buffers();
    void resize_framebuffer(int w, int h);
    void draw();

public:
    static constexpr uint32_t cmd_buf_count = 2;

private:
    Renderer& m_renderer;
    GLFWwindow* m_window = nullptr;
    View m_view {this};
    RefreshMode m_refresh_mode = RefreshMode::OnDemand;
    Color m_clear_color;
    Vec2i m_window_pos;
    Vec2i m_window_size;
    std::chrono::microseconds m_timeout {0};
    bool m_clear_timeout = false;
    bool m_draw_finished = true;

    CommandBuffers m_command_buffers;
    VkFence m_cmd_buf_fences[cmd_buf_count] {};
    VkSemaphore m_image_semaphore[cmd_buf_count] {};   // image available
    VkSemaphore m_render_semaphore[cmd_buf_count] {};  // render finished
    uint32_t m_current_cmd_buf = 0;

    UpdateCallback m_update_cb;
    SizeCallback m_size_cb;
    DrawCallback m_draw_cb;
    KeyCallback m_key_cb;
    CharCallback m_char_cb;
    MousePosCallback m_mpos_cb;
    MouseBtnCallback m_mbtn_cb;
    ScrollCallback m_scroll_cb;
};


} // namespace xci::graphics

#endif // include guard
