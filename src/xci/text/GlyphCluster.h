// GlyphCluster.h created on 2019-12-16 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_GLYPH_CLUSTER_H
#define XCI_TEXT_GLYPH_CLUSTER_H

#include <xci/graphics/Sprites.h>
#include <xci/graphics/View.h>
#include "FontFace.h"  // CodePoint

namespace xci::graphics { class Renderer; }

namespace xci::text {

using xci::graphics::ViewportCoords;
using xci::graphics::ViewportSize;

class Font;

/// GlyphCluster can be used for rendering free floating glyphs
/// or for arranging them together to make readable text.
///
/// The idea is to group the glyphs together in a single object
/// so they can be rendered efficiently, ideally by a single
/// graphics API command.
///
/// Note that all the glyphs must be rendered from the same texture,
/// which means they have to share the same Font. Each glyph can be
/// rendered with different face (from the same font) and with different
/// color.
class GlyphCluster {
public:
    /// Both objects passed to ctor must outlive GlyphCluster object.
    /// \param renderer     Renderer to be used for drawing the cluster of glyphs.
    /// \param font         Font to be used for rendering the individual glyphs.
    GlyphCluster(graphics::Renderer& renderer, Font& font);

    // ------------------------------------------------------------------------

    /// The Font object given to the constructor.
    /// It can be used to select font face and size.
    Font& font() const { return m_font; }

    void set_color(const graphics::Color& color) { m_sprites.set_color(color); }

    // ------------------------------------------------------------------------

    /// Pen is a position in page where elements are printed
    ViewportCoords pen() const { return m_pen; }

    /// Set pen to absolute viewport position
    void set_pen(ViewportCoords pen) { m_pen = pen; }

    /// Move pen relatively to its current position
    void move_pen(ViewportSize rel) { m_pen += rel; }

    // ------------------------------------------------------------------------

    /// Clear previously added glyphs.
    void clear() { m_sprites.clear(); }

    /// Reserve memory for `num` sprites.
    void reserve(size_t num) { m_sprites.reserve(num); }

    /// \param code_point   Used to look up the glyph to be renderer.
    /// \param pos          Top left corner of the positioned glyph.
    /// \param size_factor  The glyph's pixel size will be multiplied by this.
    void add_glyph(const graphics::View& view, CodePoint code_point);

    /// \param str          UTF-8 string
    void add_string(const graphics::View& view, const std::string& str);

    /// Recreate GPU objects. Call after populating all glyphs.
    void recreate() { m_sprites.update(); }

    /// Draw the glyphs. Call `recreate` before this.
    void draw(graphics::View& view, const ViewportCoords& pos) { m_sprites.draw(view, pos); }

private:
    Font& m_font;
    graphics::ColoredSprites m_sprites;
    ViewportCoords m_pen;
};


} // namespace xci::text

#endif // include guard
