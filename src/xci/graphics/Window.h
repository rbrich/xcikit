// Window.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_WINDOW_H
#define XCI_GRAPHICS_WINDOW_H

#include <xci/graphics/View.h>
#include <xci/util/geometry.h>

#include <string>
#include <memory>
#include <functional>

namespace xci {
namespace graphics {

using xci::util::Vec2u;
using xci::util::Vec2f;


enum class Key {
    A = 65,
    B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z
};
static_assert((int)Key::Z == 90);


struct KeyEvent {
    Key key;
};


class Window {
public:
    Window();
    ~Window();
    Window(Window&&) noexcept;
    Window& operator=(Window&&) noexcept;

    void create(const Vec2u& size, const std::string& title);
    void display(std::function<void(View& view)> draw_fn);

    void set_key_callback(std::function<void(View&, KeyEvent)> key_fn);

    class Impl;
    const Impl& impl() const { return *m_impl; }
    Impl* impl_ptr() { return m_impl.get(); }

private:
    std::unique_ptr<Impl> m_impl;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_WINDOW_H
