// FontTexture.cpp created on 2018-03-02, part of XCI toolkit

#include <xci/text/FontTexture.h>

#include <xci/util/log.h>
using namespace xci::util::log;

namespace xci {
namespace text {


FontTexture::FontTexture(unsigned int size_request)
{
    unsigned int size = std::min(size_request, sf::Texture::getMaximumSize());
    if (!m_texture.create(size, size))
        throw std::runtime_error("Could not create font texture.");
    m_texture.setSmooth(true);
    m_binpack.Init(size, size, /*allowFlip=*/false);
}


bool FontTexture::add_glyph(const FT_Bitmap& bitmap, sf::IntRect& coords)
{
    // empty bitmap -> zero coords
    if (bitmap.width == 0 || bitmap.rows == 0) {
        coords = {0, 0, 0, 0};
        return true;
    }

    // try to place the rect
    constexpr int p = 1;  // padding
    constexpr int pp = 2 * p;
    rbp::Rect rect = m_binpack.Insert(bitmap.width + pp, bitmap.rows + pp,
                                      rbp::MaxRectsBinPack::RectBestShortSideFit);
    if (rect.height == 0 || rect.width == 0)
        return false;
    assert(rect.width > 0);
    assert(rect.height > 0);
    assert(rect.x >= 0);
    assert(rect.y >= 0);

    // check that the bitmap is as expected (this depends
    // on FreeType settings which are under our control)
    assert(bitmap.num_grays == 256);
    assert((int)bitmap.width == bitmap.pitch);
    assert(bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);

    // transform the bitmap into 32bit RGBA format
    std::vector<uint8_t> buffer(bitmap.width * bitmap.rows * 4, 0xFF);
    for (uint i = 0; i < bitmap.width * bitmap.rows; i++) {
        buffer[4*i+3] = bitmap.buffer[i];
    }

    // copy pixels into texture
    m_texture.update(buffer.data(), bitmap.width, bitmap.rows,
                     unsigned(rect.x + p), unsigned(rect.y + p));

    // return coords
    coords = {rect.x + p, rect.y + p, (int) bitmap.width, (int) bitmap.rows};
    return true;
}


void FontTexture::clear()
{
    m_binpack.Init(m_texture.getSize().x, m_texture.getSize().y);
    std::vector<uint8_t> buffer(m_texture.getSize().x * m_texture.getSize().y * 4);
    m_texture.update(buffer.data());
}


}} // namespace xci::text
