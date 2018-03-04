// Window.cpp created on 2018-03-04, part of XCI toolkit

#include "WindowImpl.h"
#include "SpriteImpl.h"

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
    window.clear();
}

void Window::display()
{
    sf::RenderWindow& window = m_impl->window;
    window.display();
    while (window.isOpen()) {
        sf::Event event = {};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
    }
}

void Window::draw(const Sprite& sprite, const Vec2f& pos)
{
    sf::RenderStates states;
    states.transform.translate(pos.x, pos.y);
    m_impl->window.draw(sprite.impl(), states);
}


}} // namespace xci::graphics
