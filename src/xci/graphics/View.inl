namespace xci {
namespace graphics {


// Forward to View::Impl
View::View() : m_impl(new Impl) {}
View::~View() = default;
View::View(View&&) = default;
View& View::operator=(View&&) = default;


}} // namespace xci::graphics
