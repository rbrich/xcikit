// SfmlWindow.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_SFML_WINDOW_H
#define XCI_GRAPHICS_SFML_WINDOW_H

#include <xci/graphics/Window.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Window.hpp>

namespace xci {
namespace graphics {

class SfmlWindow {
public:
    void create(const Vec2u& size, const std::string& title);
    void display(std::function<void(View& view)> draw_fn);

    View create_view();

    // access native handles
    sf::RenderWindow& sfml_window() { return m_window; }

private:
    sf::RenderWindow m_window;
};

class Window::Impl : public SfmlWindow {};

}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SFML_WINDOW_H
