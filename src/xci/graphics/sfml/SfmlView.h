// SfmlView.h created on 2018-03-06, part of XCI toolkit

#ifndef XCI_GRAPHICS_SFML_VIEW_H
#define XCI_GRAPHICS_SFML_VIEW_H

#include <xci/graphics/View.h>

#include <SFML/Graphics/RenderTarget.hpp>

namespace xci {
namespace graphics {


class SfmlView {
public:
    void set_sfml_target(sf::RenderTarget& target) { m_target = &target; }
    sf::RenderTarget& sfml_target() { return *m_target; }

private:
    sf::RenderTarget* m_target;
};


class View::Impl : public SfmlView {};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_SFML_VIEW_H
