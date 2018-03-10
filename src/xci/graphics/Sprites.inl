namespace xci {
namespace graphics {


// Forward to Sprites::Impl
Sprites::Sprites(const Texture& texture) : m_impl(new Impl(texture)) {}
Sprites::~Sprites() = default;
Sprites::Sprites(Sprites&&) = default;
Sprites& Sprites::operator=(Sprites&&) = default;

void Sprites::add_sprite(const Vec2f& pos, const Color& color)
{ m_impl->add_sprite(pos, color); }

void Sprites::add_sprite(const Vec2f& pos, const Rect_u& texrect, const Color& color)
{ m_impl->add_sprite(pos, texrect, color); }

void Sprites::draw(View& view, const Vec2f& pos)
{ m_impl->draw(view, pos); }


}} // namespace xci::graphics
