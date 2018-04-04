namespace xci {
namespace graphics {


// Forward to Shapes::Impl
Shapes::Shapes(const Color& fill_color, const Color& outline_color)
        : m_impl(new Impl(fill_color, outline_color)) {}
Shapes::~Shapes() = default;
Shapes::Shapes(Shapes&&) noexcept = default;
Shapes& Shapes::operator=(Shapes&&) noexcept = default;

void Shapes::add_rectangle(const Rect_f& rect, float outline_thickness)
{ m_impl->add_rectangle(rect, outline_thickness); }

void Shapes::add_ellipse(const Rect_f& rect, float outline_thickness)
{ m_impl->add_ellipse(rect, outline_thickness); }

void Shapes::add_rounded_rectangle(const Rect_f& rect, float radius,
                                   float outline_thickness)
{ m_impl->add_rounded_rectangle(rect, radius, outline_thickness); }

void Shapes::clear() { m_impl->clear(); }

void Shapes::draw(View& view, const Vec2f& pos)
{ m_impl->draw(view, pos); }


}} // namespace xci::graphics
