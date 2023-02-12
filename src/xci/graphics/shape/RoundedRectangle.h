// RoundedRectangle.h created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_SHAPE_ROUNDED_RECTANGLE_H
#define XCI_GRAPHICS_SHAPE_ROUNDED_RECTANGLE_H

#include "Rectangle.h"
#include "Ellipse.h"

namespace xci::graphics {


/// A collection of rounded rectangle shapes.
/// Each rounded rectangle may have different size and outline thickness.
/// Colors, antialiasing and softness are uniform.
class RoundedRectangle {
public:
    explicit RoundedRectangle(Renderer& renderer) : m_rectangles(renderer), m_ellipses(renderer) {}

    /// Remove all shapes in collection
    void clear() { m_rectangles.clear(); m_ellipses.clear(); }

    /// Reserve memory for a number of rectangles.
    void reserve(size_t rounded_rectangles) {
        m_rectangles.reserve(3 * rounded_rectangles);  // 3 rectangle slices per a rounded rectangle
        m_ellipses.reserve(4 * rounded_rectangles);  // 4 ellipse slices per a rounded rectangle
    }

    /// Add new rounded rectangle.
    /// \param rect                 Position and size
    /// \param radius               Corner radius
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_rounded_rectangle(const FramebufferRect& rect, FramebufferPixels radius,
                               FramebufferPixels outline_thickness = 0);

    /// Update GPU data (vertex buffers, uniforms etc.)
    /// \param fill_color       Set fill color for all shapes in the collection.
    /// \param outline_color    Set outline color for all shapes in the collection.
    /// \param softness         What fraction of outline should be smoothed (usable range is 0.0 - 1.0).
    ///                         This is extended "antialiasing" which mixes the outline color into fill color.
    /// \param antialiasing     How many fragments should be smoothed (usable range is 0.0 - 2.0).
    void update(Color fill_color = Color::Black(), Color outline_color = Color::White(),
                float softness = 0.0f, float antialiasing = 0.0f)
    {
        m_rectangles.update(fill_color, outline_color, softness, antialiasing);
        m_ellipses.update(fill_color, outline_color, softness, antialiasing);
    }

    /// Draw all shapes to `view` at `pos`.
    /// Final shape position is `pos` + shapes' relative position
    void draw(View& view, VariCoords pos) {
        m_rectangles.draw(view, pos);
        m_ellipses.draw(view, pos);
    }

private:
    Rectangle m_rectangles;
    Ellipse m_ellipses;
};


/// A collection of rounded rectangle shapes.
/// Each rounded rectangle may have different size, color and outline thickness.
/// Antialiasing and softness is uniform.
class ColoredRoundedRectangle {
public:
    explicit ColoredRoundedRectangle(Renderer& renderer) : m_rectangles(renderer), m_ellipses(renderer) {}

    /// Remove all shapes in collection
    void clear() { m_rectangles.clear(); m_ellipses.clear(); }

    /// Reserve memory for a number of rectangles.
    void reserve(size_t rounded_rectangles) {
        m_rectangles.reserve(3 * rounded_rectangles);  // 3 rectangle slices per a rounded rectangle
        m_ellipses.reserve(4 * rounded_rectangles);  // 4 ellipse slices per a rounded rectangle
    }

    /// Add new rounded rectangle.
    /// \param rect                 Position and size
    /// \param radius               Corner radius
    /// \param fill_color           Fill color
    /// \param outline_color        Outline color
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_rounded_rectangle(const FramebufferRect& rect, FramebufferPixels radius,
                   Color fill_color, Color outline_color, FramebufferPixels outline_thickness = 0);

    /// Update GPU data (vertex buffers, uniforms etc.)
    /// \param softness         What fraction of outline should be smoothed (usable range is 0.0 - 1.0).
    ///                         This is extended "antialiasing" which mixes the outline color into fill color.
    /// \param antialiasing     How many fragments should be smoothed (usable range is 0.0 - 2.0).
    void update(float softness = 0.0f, float antialiasing = 0.0f) {
        m_rectangles.update(softness, antialiasing);
        m_ellipses.update(softness, antialiasing);
    }

    /// Draw all shapes to `view` at `pos`.
    /// Final shape position is `pos` + shapes' relative position
    void draw(View& view, VariCoords pos) {
        m_rectangles.draw(view, pos);
        m_ellipses.draw(view, pos);
    }

private:
    ColoredRectangle m_rectangles;
    ColoredEllipse m_ellipses;
};


/// Convenience - build rounded rectangles in resize() method with any units
class RoundedRectangleBuilder: public UniformColorShapeBuilder<RoundedRectangleBuilder> {
public:
    RoundedRectangleBuilder(View& view, RoundedRectangle& shape) : m_view(view), m_shape(shape) { m_shape.clear(); }
    ~RoundedRectangleBuilder() { m_shape.update(m_fill_color, m_outline_color, m_softness, m_antialiasing); }

    RoundedRectangleBuilder&
    add_rounded_rectangle(const VariRect& rect, VariUnits radius, VariUnits outline_thickness = {})
    {
        m_shape.add_rounded_rectangle(m_view.to_fb(rect), m_view.to_fb(radius), m_view.to_fb(outline_thickness));
        return *this;
    }

private:
    View& m_view;
    RoundedRectangle& m_shape;
};


/// Convenience - build colored rounded rectangles in resize() method with any units
class ColoredRoundedRectangleBuilder: public VaryingColorShapeBuilder<ColoredRoundedRectangleBuilder> {
public:
    ColoredRoundedRectangleBuilder(View& view, ColoredRoundedRectangle& shape) : m_view(view), m_shape(shape) { m_shape.clear(); }
    ~ColoredRoundedRectangleBuilder() { m_shape.update(m_softness, m_antialiasing); }

    ColoredRoundedRectangleBuilder&
    add_rounded_rectangle(const VariRect& rect, VariUnits radius,
                  Color fill_color, Color outline_color, VariUnits outline_thickness = {})
    {
        m_shape.add_rounded_rectangle(m_view.to_fb(rect), m_view.to_fb(radius),
                                  fill_color, outline_color, m_view.to_fb(outline_thickness));
        return *this;
    }

private:
    View& m_view;
    ColoredRoundedRectangle& m_shape;
};


} // namespace xci::graphics

#endif // XCI_GRAPHICS_SHAPE_ROUNDED_RECTANGLE_H
