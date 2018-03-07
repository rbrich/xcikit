// ViewImpl.h created on 2018-03-06, part of XCI toolkit

#ifndef XCI_GRAPHICS_SFML_VIEWIMPL_H
#define XCI_GRAPHICS_SFML_VIEWIMPL_H

#include <xci/graphics/View.h>

#include <SFML/Graphics/RenderTarget.hpp>

namespace xci {
namespace graphics {


class ViewImpl {
public:
    void draw(sf::RenderTarget& target);

    sf::RenderTarget* target;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SFML_VIEWIMPL_H
