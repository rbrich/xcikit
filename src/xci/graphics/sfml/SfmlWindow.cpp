// SfmlWindow.cpp created on 2018-03-04, part of XCI toolkit

#include "SfmlWindow.h"
#include "SfmlSprites.h"
#include "SfmlView.h"

#include <SFML/Window/Event.hpp>

// inline
#include <xci/graphics/Window.inl>

namespace xci {
namespace graphics {


void SfmlWindow::create(const Vec2u& size, const std::string& title)
{
    m_window.create({size.x, size.y}, title);
    sf::View view;
    view.setCenter(0, 0);
    view.setSize(size.x, size.y);
    m_window.setView(view);
}

void SfmlWindow::display(std::function<void(View& view)> draw_fn)
{
    View view = create_view();
    while (m_window.isOpen()) {
        sf::Event event = {};
        while (m_window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                m_window.close();
        }
        m_window.clear();
        draw_fn(view);
        m_window.display();
    }
}

View SfmlWindow::create_view()
{
    View view;
    view.impl().set_sfml_target(m_window);
    return view;
}


}} // namespace xci::graphics
