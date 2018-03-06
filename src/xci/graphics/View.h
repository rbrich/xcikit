// View.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_VIEW_H
#define XCI_GRAPHICS_VIEW_H

namespace xci {
namespace graphics {

class ViewImpl;

class View
{
public:
    View();
    ~View();

    ViewImpl& impl() const { return *m_impl; }

private:
    ViewImpl* m_impl;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_VIEW_H
