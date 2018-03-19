namespace xci {
namespace graphics {


// Forward to Window::Impl
Window::Window() : m_impl(new Impl) {}
Window::~Window() = default;
Window::Window(Window&&) noexcept = default;
Window& Window::operator=(Window&&) noexcept = default;

void Window::create(const Vec2u& size, const std::string& title) { m_impl->create(size, title); }
void Window::display(std::function<void(View& view)> draw_fn) { m_impl->display(std::move(draw_fn)); }


}} // namespace xci::graphics
