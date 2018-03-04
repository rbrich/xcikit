// View.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_VIEW_H
#define XCI_GRAPHICS_VIEW_H

#include <xci/graphics/Sprite.h>

#include <xci/util/geometry.h>
using xci::util::Vec2f;

namespace xci {
namespace graphics {


class View
{
public:
    virtual ~View() {}

    virtual void draw(const Sprite& sprite, const Vec2f& pos) = 0;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_VIEW_H
