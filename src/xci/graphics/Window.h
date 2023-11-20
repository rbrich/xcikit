// Window.h created on 2018-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_WINDOW_H
#define XCI_GRAPHICS_WINDOW_H

#include "View.h"
#include <xci/graphics/vulkan/CommandBuffers.h>
#include <xci/graphics/Color.h>
#include <xci/math/Vec2.h>
#include <xci/core/mixin.h>

#include <vulkan/vulkan.h>

#include <string>
#include <memory>
#include <functional>
#include <utility>
#include <chrono>

struct SDL_Window;
union SDL_Event;

namespace xci::graphics {

using xci::core::Vec2u;
using xci::core::Vec2f;
using xci::core::Vec2i;

// Key names come from GLFW, with only minor changes
enum class Key: uint8_t {
    Unknown = 0,

    // The following key codes correspond to ASCII
    Backspace = '\b',
    Tab = '\t',
    Return = '\r',
    Escape = '\x1b',
    Space = ' ',
    Apostrophe  = '\'',
    Comma = ',',
    Minus = '-',
    Period = '.',
    Slash = '/',
    Num0 = '0', Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Semicolon = ';',
    Equal = '=',
    A = 'A', B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftBracket = '[',
    Backslash = '\\',
    RightBracket = ']',
    Backtick = '`',

    // Function keys
    F1 = 128, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Insert, Delete,
    Home, End,
    PageUp, PageDown,
    Left, Right, Up, Down,
    CapsLock, ScrollLock, NumLock,
    PrintScreen,
    Pause,

    // Numeric keypad
    Keypad0, Keypad1, Keypad2, Keypad3, Keypad4, Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
    KeypadPlus, KeypadMinus, KeypadMultiply, KeypadDivide, KeypadDecimalPoint, KeypadEnter,

    // Modifier keys
    LeftShift, RightShift,
    LeftControl, RightControl,
    LeftAlt, RightAlt,
    LeftSuper, RightSuper,  // Command or Windows key
    Menu,                   // Windows menu key
};
static_assert((int)Key::Z == 90, "ascii letters");


struct ModKey {
    bool shift : 1 = false;
    bool ctrl : 1 = false;
    bool alt : 1 = false;
    bool super : 1 = false;

    static constexpr ModKey None() { return {false, false, false}; }
    static constexpr ModKey Shift() { return {true, false, false}; }
    static constexpr ModKey Ctrl() { return {false, true, false}; }
    static constexpr ModKey Alt() { return {false, false, true}; }
    static constexpr ModKey ShiftCtrl() { return {true, true, false}; }
    static constexpr ModKey ShiftAlt() { return {true, false, true}; }
    static constexpr ModKey CtrlAlt() { return {false, true, true}; }
    static constexpr ModKey ShiftCtrlAlt() { return {true, true, true}; }

    bool operator== (ModKey other) const
        { return shift == other.shift && ctrl == other.ctrl && alt == other.alt && super == other.super;}
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


struct TextInputEvent {
    char text[32];
    int edit_cursor = -1;  // >= 0 for text editing event, see SDL_TextEditingEvent
    int edit_length = 0;

    bool is_ime_edit() const { return edit_cursor != -1; }
};


enum class MouseButton {
    Left = 1,
    Middle = 2,
    Right = 3,
    Ext1 = 4,
    Ext2 = 5,
};

struct MousePosEvent {
    FramebufferCoords pos; // FIXME: ScreenCoords
    ScreenCoords rel;  // relative movement in screen coordinates
};

struct MouseBtnEvent {
    MouseButton button;
    Action action;
    FramebufferCoords pos;  // FIXME: ScreenCoords
};


struct ScrollEvent {
    Vec2f offset;
};


enum class RefreshMode {
    OnDemand,   // got refresh event from system or called View::refresh()
    OnEvent,    // got any event from system
    Periodic,   // continuous refresh
};

enum class FullscreenMode: uint8_t {
    Default,    // sane default for the platform
    Exclusive,  // normal fullscreen mode (with mode switching, but keep the desktop mode)
    Desktop,    // on Mac, this creates a new space (similar to what system fullscreen button does)
    BorderlessWindow,  // maximized borderless window
};


class Renderer;


class Window: private core::NonCopyable {
public:
    explicit Window(Renderer& renderer);
    ~Window();

    Renderer& renderer() const;

    // Create the window.
    [[nodiscard]] bool create(const Vec2u& size, const std::string& title);

    // Run main loop. Doesn't exit until the window is closed.
    void display();

    // When in OnDemand or OnEvent refresh mode, call this to wake up
    // event loop. Put custom handler into UpdateCallback.
    // (thread-safe)
    void wakeup() const;

    // Stop the main loop and close the window.
    // (thread-safe)
    void close();

    // Toggle fullscreen / windowed mode
    // The window starts in windowed mode.
    void toggle_fullscreen() { set_fullscreen(!m_fullscreen); }
    void set_fullscreen(bool fullscreen);
    bool is_fullscreen() const { return m_fullscreen; }
    void set_fullscreen_mode(FullscreenMode mode) { m_fullscreen_mode = mode; }

    // Set clipboard text (in UTF-8)
    bool set_clipboard_text(const std::string& text) const;
    // Get clipboard text (in UTF-8)
    std::string get_clipboard_text() const;

    using UpdateCallback = std::function<void(View&, std::chrono::nanoseconds elapsed)>;
    using SizeCallback = std::function<void(View&)>;
    using DrawCallback = std::function<void(View&)>;
    using KeyCallback = std::function<void(View&, const KeyEvent&)>;
    using TextInputCallback = std::function<void(View&, const TextInputEvent&)>;
    using MousePosCallback = std::function<void(View&, const MousePosEvent&)>;
    using MouseBtnCallback = std::function<void(View&, const MouseBtnEvent&)>;
    using ScrollCallback = std::function<void(View&, const ScrollEvent&)>;

    // The original callback is replaced. To cascade callbacks,
    // you have to get and wrap original callback manually.
    void set_update_callback(UpdateCallback update_cb) { m_update_cb = std::move(update_cb); }
    void set_size_callback(SizeCallback size_cb) { m_size_cb = std::move(size_cb); }
    void set_draw_callback(DrawCallback draw_cb);
    void set_key_callback(KeyCallback key_cb) { m_key_cb = std::move(key_cb); }
    void set_text_input_callback(TextInputCallback text_cb) { m_text_cb = std::move(text_cb); }
    void set_mouse_position_callback(MousePosCallback mpos_cb) { m_mpos_cb = std::move(mpos_cb); }
    void set_mouse_button_callback(MouseBtnCallback mbtn_cb) { m_mbtn_cb = std::move(mbtn_cb); }
    void set_scroll_callback(ScrollCallback scroll_cb) { m_scroll_cb = std::move(scroll_cb); }

    UpdateCallback update_callback() { return m_update_cb; }
    SizeCallback size_callback() { return m_size_cb; }
    DrawCallback draw_callback() { return m_draw_cb; }
    KeyCallback key_callback() { return m_key_cb; }
    TextInputCallback text_input_callback() { return m_text_cb; }
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
    RefreshMode refresh_mode() const { return m_refresh_mode; }

    /// Set refresh timeout. This is useful for OnDemand/OnEvent modes,
    /// where no update events are generated unless an event occurs.
    /// When configured, the update is called at most after `timetout`,
    /// but possibly sooner if something else happens.
    /// \param timeout      The timeout in usecs. Zero disables the timeout.
    /// \param periodic     False = one-shot (timeout is cleared after next update).
    ///                     True = periodic (no clear).
    void set_refresh_timeout(std::chrono::microseconds timeout, bool periodic);

    /// Set origin of the coordinates to be used throughout the program
    /// for placing elements in the view.
    /// \param origin       The position of (0,0) coordinates. Default is Center.
    void set_view_origin(ViewOrigin origin);

    void set_debug_flags(View::DebugFlags flags);

    /// Wait for asynchronous draw commands to finish.
    /// This needs to be called before recreating objects that are being drawn.
    void finish_draw();

    // SDL handles
    SDL_Window* sdl_window() const { return m_window; }

    // Vulkan - current command buffer
    VkCommandBuffer vk_command_buffer() const { return m_command_buffers[m_current_cmd_buf]; }
    uint32_t command_buffer_index() const { return m_current_cmd_buf; }
    void add_command_buffer_resource(const ResourcePtr& resource) { m_command_buffers.add_resource(m_current_cmd_buf, resource); }

private:
    void setup_view();
    void create_command_buffers();
    void resize_framebuffer();
    void draw();
    void handle_event(const SDL_Event& event);

public:
    static constexpr uint32_t cmd_buf_count = 2;

private:
    Renderer& m_renderer;
    SDL_Window* m_window = nullptr;
    View m_view {this};
    RefreshMode m_refresh_mode = RefreshMode::OnDemand;
    Color m_clear_color;
    std::chrono::microseconds m_timeout {0};
    FullscreenMode m_fullscreen_mode = FullscreenMode::Default;
    bool m_quit = false;
    bool m_fullscreen = false;
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
    TextInputCallback m_text_cb;
    MousePosCallback m_mpos_cb;
    MouseBtnCallback m_mbtn_cb;
    ScrollCallback m_scroll_cb;
};


} // namespace xci::graphics

#endif // include guard
