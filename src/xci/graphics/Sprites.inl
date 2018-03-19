namespace xci {
namespace graphics {


// Forward to Sprites::Impl
Sprites::Sprites(const Texture& texture, const Color& color)
        : m_impl(new Impl(texture, color)) {}
Sprites::~Sprites() = default;
Sprites::Sprites(Sprites&&) noexcept = default;
Sprites& Sprites::operator=(Sprites&&) noexcept = default;

void Sprites::add_sprite(const Rect_f& rect)
{ m_impl->add_sprite(rect); }

void Sprites::add_sprite(const Rect_f& rect, const Rect_u& texrect)
{ m_impl->add_sprite(rect, texrect); }

void Sprites::draw(View& view, const Vec2f& pos)
{ m_impl->draw(view, pos); }


}} // namespace xci::graphics
