namespace xci {
namespace graphics {


// Forward to Window::Impl
Window::Window() : m_impl(new Impl) {}
Window::~Window() = default;
Window::Window(Window&&) = default;
Window& Window::operator=(Window&&) = default;

void Window::create(const Vec2u& size, const std::string& title) { m_impl->create(size, title); }
void Window::display() { m_impl->display(); }
View Window::create_view() { return m_impl->create_view(); }


}} // namespace xci::graphics
