// View.cpp created on 2018-03-06, part of XCI toolkit

#include "ViewImpl.h"

namespace xci {
namespace graphics {


View::View() : m_impl(new ViewImpl) {}
View::~View() { delete m_impl; }


}} // namespace xci::graphics
