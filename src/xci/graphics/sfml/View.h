// View.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_SFML_VIEW_H
#define XCI_GRAPHICS_SFML_VIEW_H

#include <xci/graphics/Sprite.h>

#include <xci/util/geometry.h>
using xci::util::Vec2f;

#include <SFML/Graphics.hpp>

namespace xci {
namespace graphics {

class View
{
public:

    void set_size(const Vec2u& size) {
        sfml_mode.width = size.x;
        sfml_mode.height = size.y;
        sfml_mode.bitsPerPixel = 32;
    }

    void create_window(const std::string& title) {
        sfml_window.create(sfml_mode, title);
        sf::View view;
        view.setCenter(0, 0);
        view.setSize(sfml_mode.width, sfml_mode.height);
        sfml_window.setView(view);
        sfml_window.clear();
    }

    void display() {
        sfml_window.display();
        while (sfml_window.isOpen()) {
            sf::Event event = {};
            while (sfml_window.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    sfml_window.close();
            }
        }
    }

    void draw(const Sprite& sprite, const Vec2f& pos) {
        sf::RenderStates states;
        states.transform.translate(pos.x, pos.y);
        sfml_window.draw(sprite.sfml(), states);
    }

private:
    sf::RenderWindow sfml_window;
    sf::VideoMode sfml_mode;
};

}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SFML_VIEW_H
