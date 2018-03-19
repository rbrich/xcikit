namespace xci {
namespace graphics {


// Forward to Rectangles::Impl
Rectangles::Rectangles(const Color& fill_color, const Color& outline_color)
        : m_impl(new Impl(fill_color, outline_color)) {}
Rectangles::~Rectangles() = default;
Rectangles::Rectangles(Rectangles&&) noexcept = default;
Rectangles& Rectangles::operator=(Rectangles&&) noexcept = default;

void Rectangles::add_rectangle(const Rect_f& rect, float outline_width)
{ m_impl->add_rectangle(rect, outline_width); }


void Rectangles::draw(View& view, const Vec2f& pos)
{ m_impl->draw(view, pos); }


}} // namespace xci::graphics
