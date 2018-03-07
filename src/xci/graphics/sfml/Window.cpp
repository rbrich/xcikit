// Window.cpp created on 2018-03-04, part of XCI toolkit

#include "WindowImpl.h"
#include "SpritesImpl.h"
#include "ViewImpl.h"

#include <SFML/Window/Event.hpp>

namespace xci {
namespace graphics {

Window::Window() : m_impl(new WindowImpl) {}
Window::~Window() { delete m_impl; }

void Window::create(const Vec2u& size, const std::string& title)
{
    sf::RenderWindow& window = m_impl->window;
    window.create({size.x, size.y}, title);
    sf::View view;
    view.setCenter(0, 0);
    view.setSize(size.x, size.y);
    window.setView(view);
}

void Window::display()
{
    sf::RenderWindow& window = m_impl->window;
    while (window.isOpen()) {
        sf::Event event = {};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        window.clear();
        // TODO: draw view
        window.display();
    }
}

View Window::create_view()
{
    View view;
    view.impl().target = &m_impl->window;
    return view;
}


}} // namespace xci::graphics
