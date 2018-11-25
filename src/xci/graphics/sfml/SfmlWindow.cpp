// SfmlWindow.cpp created on 2018-03-04, part of XCI toolkit

#include "SfmlWindow.h"
#include "SfmlPrimitives.h"
#include <xci/core/log.h>

#include <SFML/Window/Event.hpp>

#include <chrono>

namespace xci::graphics {

using namespace xci::core::log;
using namespace std::chrono;


Window& Window::default_instance()
{
    static SfmlWindow window;
    return window;
}


void SfmlWindow::create(const Vec2u& size, const std::string& title)
{
    sf::ContextSettings settings;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Core;

    m_window.create({size.x, size.y}, title, sf::Style::Default, settings);
    sf::View view;
    view.setCenter(0, 0);
    view.setSize(size.x, size.y);
    m_window.setView(view);
    m_window.setActive(true);

    if (!gladLoadGL()) {
        log_error("Couldn't initialize OpenGL...");
        exit(1);
    }
    log_info("OpenGL {} GLSL {}",
             glGetString(GL_VERSION),
             glGetString(GL_SHADING_LANGUAGE_VERSION));
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
    auto wsize = m_window.getSize();
    glViewport(0, 0, wsize.x, wsize.y);
    m_view.set_framebuffer_size({wsize.x, wsize.y});
    m_view.set_screen_size({wsize.x, wsize.y});
    if (m_size_cb)
        m_size_cb(m_view);

}


void SfmlWindow::handle_event(sf::Event& event)
{
    switch (event.type) {
        case sf::Event::Closed:
            m_window.close();
            break;
        case sf::Event::Resized:
            glViewport(0, 0, event.size.width, event.size.height);
            m_view.set_framebuffer_size({event.size.width, event.size.height});
            m_view.set_screen_size({event.size.width, event.size.height});
            if (m_size_cb)
                m_size_cb(m_view);
            break;
        default:
            break;
    }
}


void SfmlWindow::draw()
{
    m_window.clear();
    if (m_draw_cb)
        m_draw_cb(m_view);
    m_window.display();
}


} // namespace xci::graphics
