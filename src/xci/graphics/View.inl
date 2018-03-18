namespace xci {
namespace graphics {


// Forward to View::Impl
View::View(Vec2u pixel_size) : m_impl(new Impl(pixel_size)) {}
View::~View() = default;
View::View(View&&) = default;
View& View::operator=(View&&) = default;

void View::resize(Vec2u pixel_size) { m_impl->resize(pixel_size); }

Vec2f View::size() const { return m_impl->size(); }
Vec2u View::pixel_size() const { return m_impl->pixel_size(); }


}} // namespace xci::graphics
