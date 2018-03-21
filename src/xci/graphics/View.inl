namespace xci {
namespace graphics {


// Forward to View::Impl
View::View(Vec2u pixel_size) : m_impl(new Impl(pixel_size)) {}
View::~View() = default;
View::View(View&&) noexcept = default;
View& View::operator=(View&&) noexcept = default;

void View::resize(Vec2u pixel_size) { m_impl->resize(pixel_size); }

Vec2f View::size() const { return m_impl->size(); }
Vec2u View::pixel_size() const { return m_impl->pixel_size(); }


void View::set_debug_flag(View::Debug flag, bool enabled) {
    if (has_debug_flag(flag) != enabled)
        m_debug ^= (DebugFlags) flag;
}

bool View::has_debug_flag(View::Debug flag) const {
    return bool(m_debug & (DebugFlags)flag);
}


}} // namespace xci::graphics
