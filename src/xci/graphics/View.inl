namespace xci {
namespace graphics {


// Forward to View::Impl
View::View() : m_impl(new Impl) {}
View::~View() = default;
View::View(View&&) noexcept = default;
View& View::operator=(View&&) noexcept = default;

void View::set_screen_size(Vec2u size) { m_impl->set_screen_size(size); }
Vec2u View::screen_size() const { return m_impl->screen_size(); }

void View::set_framebuffer_size(Vec2u size) { m_impl->set_framebuffer_size(size); }
Vec2u View::framebuffer_size() const { return m_impl->framebuffer_size(); }

Vec2f View::scalable_size() const { return m_impl->scalable_size(); }


void View::set_debug_flag(View::Debug flag, bool enabled) {
    if (has_debug_flag(flag) != enabled)
        m_debug ^= (DebugFlags) flag;
}

bool View::has_debug_flag(View::Debug flag) const {
    return bool(m_debug & (DebugFlags)flag);
}


}} // namespace xci::graphics
