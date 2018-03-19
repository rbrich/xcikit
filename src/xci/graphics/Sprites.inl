namespace xci {
namespace graphics {


// Forward to Sprites::Impl
Sprites::Sprites(const Texture& texture) : m_impl(new Impl(texture)) {}
Sprites::~Sprites() = default;
Sprites::Sprites(Sprites&&) noexcept = default;
Sprites& Sprites::operator=(Sprites&&) noexcept = default;

void Sprites::add_sprite(const Rect_f& rect, const Color& color)
{ m_impl->add_sprite(rect, color); }

void Sprites::add_sprite(const Rect_f& rect, const Rect_u& texrect, const Color& color)
{ m_impl->add_sprite(rect, texrect, color); }

void Sprites::draw(View& view, const Vec2f& pos)
{ m_impl->draw(view, pos); }


}} // namespace xci::graphics
