// Window.h created on 2018-03-04, part of XCI toolkit
// Copyright 2018 Radek Brich
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

#ifndef XCI_GRAPHICS_WINDOW_H
#define XCI_GRAPHICS_WINDOW_H

#include <xci/graphics/View.h>
#include <xci/util/geometry.h>

#include <string>
#include <memory>
#include <functional>
#include <utility>
#include <chrono>

namespace xci {
namespace graphics {

using xci::util::Vec2u;
using xci::util::Vec2f;


enum class Key {
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
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftBracket = 91, Backslash, RightBracket,  // [ \ ]
    GraveAccent = 96,  // `

    // Numeric keypad
    Keypad0 = 128, Keypad1, Keypad2, Keypad3, Keypad4, Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
    KeypadPlus, KeypadMinus, KeypadAsterisk, KeypadSlash, KeypadDecimalPoint, KeypadEnter,
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
    Vec2f pos;  // scalable coordinates

    MousePosEvent() = delete;
};

struct MouseBtnEvent {
    MouseButton button;
    Action action;
    Vec2f pos;  // scalable coordinates

    MouseBtnEvent() = delete;
};


struct ScrollEvent {
    Vec2f offset;
};


enum class RefreshMode {
    OnDemand,  // refresh event from system or through View::refresh()
    OnEvent,   // got any event from system
    PeriodicVsync,
    PeriodicNoWait,
};


class Window {
public:
    static Window& default_window();
    virtual ~Window() = default;

    // Create the window.
    virtual void create(const Vec2u& size, const std::string& title) = 0;

    // Run main loop. Doesn't exit until the window is closed.
    virtual void display() = 0;

    // When in OnDemand or OnEvent refresh mode, call this to wake up
    // event loop. Put custom handler into UpdateCallback.
    // (thread-safe)
    virtual void wakeup() const = 0;

    // Stop the main loop and close the window.
    // (thread-safe)
    virtual void close() const = 0;

    virtual void set_clipboard_string(const std::string& s) const = 0;
    virtual std::string get_clipboard_string() const = 0;

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
    virtual void set_update_callback(UpdateCallback update_cb) { m_update_cb = std::move(update_cb); }
    virtual void set_size_callback(SizeCallback size_cb) { m_size_cb = std::move(size_cb); }
    virtual void set_draw_callback(DrawCallback draw_cb) { m_draw_cb = std::move(draw_cb); }
    virtual void set_key_callback(KeyCallback key_cb) { m_key_cb = std::move(key_cb); }
    virtual void set_char_callback(CharCallback char_cb) { m_char_cb = std::move(char_cb); }
    virtual void set_mouse_position_callback(MousePosCallback mpos_cb) { m_mpos_cb = std::move(mpos_cb); }
    virtual void set_mouse_button_callback(MouseBtnCallback mbtn_cb) { m_mbtn_cb = std::move(mbtn_cb); }
    virtual void set_scroll_callback(ScrollCallback scroll_cb) { m_scroll_cb = std::move(scroll_cb); }

    UpdateCallback get_update_callback() { return m_update_cb; }
    SizeCallback get_size_callback() { return m_size_cb; }
    DrawCallback get_draw_callback() { return m_draw_cb; }
    KeyCallback get_key_callback() { return m_key_cb; }
    CharCallback get_char_callback() { return m_char_cb; }
    MousePosCallback get_mouse_position_callback() { return m_mpos_cb; }
    MouseBtnCallback get_mouse_button_callback() { return m_mbtn_cb; }
    ScrollCallback get_scroll_callback() { return m_scroll_cb; }

    virtual void set_refresh_mode(RefreshMode mode) = 0;
    virtual void set_debug_flags(View::DebugFlags flags) = 0;

protected:
    UpdateCallback m_update_cb;
    SizeCallback m_size_cb;
    DrawCallback m_draw_cb;
    KeyCallback m_key_cb;
    CharCallback m_char_cb;
    MousePosCallback m_mpos_cb;
    MouseBtnCallback m_mbtn_cb;
    ScrollCallback m_scroll_cb;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_WINDOW_H
