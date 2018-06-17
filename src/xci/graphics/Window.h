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
    Space,
    Num0 = 48,
    Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    A = 65,
    B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
};
static_assert((int)Key::Z == 90, "ascii letters");


struct ModKey {
    bool shift : 1;
    bool ctrl : 1;
    bool alt : 1;

    bool none() const { return !(shift || ctrl || alt); }
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

    virtual void create(const Vec2u& size, const std::string& title) = 0;
    virtual void display() = 0;

    using SizeCallback = std::function<void(View&)>;
    using DrawCallback = std::function<void(View&)>;
    using KeyCallback = std::function<void(View&, const KeyEvent&)>;
    using CharCallback = std::function<void(View&, const CharEvent&)>;
    using MousePosCallback = std::function<void(View&, const MousePosEvent&)>;
    using MouseBtnCallback = std::function<void(View&, const MouseBtnEvent&)>;

    virtual void set_size_callback(SizeCallback size_cb) = 0;
    virtual void set_draw_callback(DrawCallback draw_cb) = 0;
    virtual void set_key_callback(KeyCallback key_cb) = 0;
    virtual void set_char_callback(CharCallback char_cb) = 0;
    virtual void set_mouse_position_callback(MousePosCallback mpos_cb) = 0;
    virtual void set_mouse_button_callback(MouseBtnCallback mbtn_cb) = 0;

    virtual void set_refresh_mode(RefreshMode mode) = 0;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_WINDOW_H
