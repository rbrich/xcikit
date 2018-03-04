// WindowImpl.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_SFML_WINDOWIMPL_H
#define XCI_GRAPHICS_SFML_WINDOWIMPL_H

#include <xci/graphics/Window.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Window.hpp>

namespace xci {
namespace graphics {

class WindowImpl {
public:
    sf::RenderWindow window;
};

}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SFML_WINDOWIMPL_H
