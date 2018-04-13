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

namespace xci {
namespace graphics {

using xci::util::Vec2u;
using xci::util::Vec2f;


enum class Key {
    A = 65,
    B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z
};
static_assert((int)Key::Z == 90, "ascii letters");


struct KeyEvent {
    Key key;
};


enum class MouseButton { Left = 0, Right = 1, Middle = 2 };
enum class Action { Release = 0, Press = 1 };

struct MouseEvent {
    MouseButton button;
    Action action;
    Vec2f pos;  // scalable coordinates
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

    using MousePosCallback = std::function<void(View&, const Vec2f&)>;
    using MouseBtnCallback = std::function<void(View&, const MouseEvent&)>;

    virtual void set_size_callback(std::function<void(View&)> size_cb) = 0;
    virtual void set_draw_callback(std::function<void(View& )> draw_cb) = 0;
    virtual void set_key_callback(std::function<void(View&, KeyEvent)> key_cb) = 0;
    virtual void set_mouse_position_callback(MousePosCallback mpos_cb) = 0;
    virtual void set_mouse_button_callback(MouseBtnCallback mbtn_cb) = 0;

    virtual void set_refresh_mode(RefreshMode mode) = 0;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_WINDOW_H
