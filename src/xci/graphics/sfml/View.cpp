// View.cpp created on 2018-03-06, part of XCI toolkit

#include "ViewImpl.h"

namespace xci {
namespace graphics {


View::View() : m_impl(new ViewImpl) {}
View::~View() { delete m_impl; }


void ViewImpl::draw(sf::RenderTarget& target)
{
 /*   sf::RenderStates states;
    states.transform.translate(pos.x, pos.y);
    target.draw(sprite.impl(), states);*/
}


}} // namespace xci::graphics
