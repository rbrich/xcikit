// SfmlWindow.h created on 2018-03-04, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_SFML_WINDOW_H
#define XCI_GRAPHICS_SFML_WINDOW_H

#include <xci/graphics/Window.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Window.hpp>

namespace xci::graphics {

class SfmlWindow: public Window {
public:
    void create(const Vec2u& size, const std::string& title) override;
    void display() override;
    void wakeup() const override { /* TODO */ }
    void close() const override { /* TODO */ }

    void set_clipboard_string(const std::string& s) const override { /* TODO */ }
    std::string get_clipboard_string() const override { /* TODO */ return ""; }

    void set_refresh_mode(RefreshMode mode) override { m_mode = mode; }
    void set_refresh_interval(int interval) override {}
    void set_debug_flags(View::DebugFlags flags) override { m_view.set_debug_flags(flags); }

    // access native object
    sf::RenderWindow& sfml_window() { return m_window; }

private:
    void setup_view();
    void handle_event(sf::Event& event);
    void draw();

private:
    sf::RenderWindow m_window;

    View m_view {this};
    RefreshMode m_mode = RefreshMode::OnDemand;
};

} // namespace xci::graphics

#endif // XCI_GRAPHICS_SFML_WINDOW_H
