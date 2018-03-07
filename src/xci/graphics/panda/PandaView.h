// PandaView.h created on 2018-03-06, part of XCI toolkit

#ifndef XCI_GRAPHICS_PANDA_VIEW_H
#define XCI_GRAPHICS_PANDA_VIEW_H

#include <xci/graphics/View.h>

#include <pandaFramework.h>

namespace xci {
namespace graphics {


class PandaView {
public:
    NodePath root_node;
};


class View::Impl : public PandaView {};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_PANDA_VIEW_H
