// View.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_VIEW_H
#define XCI_GRAPHICS_VIEW_H

#include <memory>

namespace xci {
namespace graphics {


class View
{
public:
    View();
    ~View();
    View(View&&);
    View& operator=(View&&);

    class Impl;
    Impl& impl() { return *m_impl; }

private:
    std::unique_ptr<Impl> m_impl;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_VIEW_H
