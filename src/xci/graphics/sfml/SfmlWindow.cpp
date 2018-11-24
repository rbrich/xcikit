// SfmlWindow.cpp created on 2018-03-04, part of XCI toolkit

#include "SfmlWindow.h"
#include "SfmlSprites.h"

#include <SFML/Window/Event.hpp>

#include <chrono>

namespace xci::graphics {

using namespace std::chrono;


void SfmlWindow::create(const Vec2u& size, const std::string& title)
{
    m_window.create({size.x, size.y}, title);
    sf::View view;
    view.setCenter(0, 0);
    view.setSize(size.x, size.y);
    m_window.setView(view);
}


void SfmlWindow::display()
{
    setup_view();

    auto t_last = steady_clock::now();
    while (m_window.isOpen()) {
        if (m_update_cb) {
            auto t_now = steady_clock::now();
            m_update_cb(m_view, t_now - t_last);
            t_last = t_now;
        }

        sf::Event event = {};
        switch (m_mode) {
            case RefreshMode::OnDemand:
                if (m_view.pop_refresh())
                    draw();
                if (m_window.waitEvent(event)) {
                    handle_event(event);
                    while (m_window.pollEvent(event))
                        handle_event(event);
                }
                break;
            case RefreshMode::OnEvent:
                draw();
                if (m_window.waitEvent(event)) {
                    handle_event(event);
                    while (m_window.pollEvent(event))
                        handle_event(event);
                }
                break;
            case RefreshMode::Periodic:
                draw();
                while (m_window.pollEvent(event))
                    handle_event(event);
                break;
        }
    }
}


void SfmlWindow::setup_view()
{
}


void SfmlWindow::handle_event(sf::Event& event)
{
    if (event.type == sf::Event::Closed)
        m_window.close();
}


void SfmlWindow::draw()
{
    m_window.clear();
    if (m_draw_cb)
        m_draw_cb(m_view);
    m_window.display();
}


} // namespace xci::graphics
