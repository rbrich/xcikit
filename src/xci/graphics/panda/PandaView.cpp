// PandaView.cpp created on 2018-03-06, part of XCI toolkit

#include "PandaView.h"

// inline
#include <xci/graphics/View.inl>

namespace xci {
namespace graphics {


View create_view_from_nodepath(NodePath node)
{
    View view;
    view.impl().root_node = node;
    return view;
}


}} // namespace xci::graphics
