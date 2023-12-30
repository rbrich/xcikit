// RoundedRectangle.h created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_SHAPE_ROUNDED_RECTANGLE_H
#define XCI_GRAPHICS_SHAPE_ROUNDED_RECTANGLE_H

#include "Shape.h"

namespace xci::graphics {


/// A collection of rounded rectangle shapes.
/// Each rounded rectangle may have different size and outline thickness.
/// Colors, antialiasing and softness are uniform.
class RoundedRectangle: public UniformColorShape {
public:
    explicit RoundedRectangle(Renderer& renderer)
    : UniformColorShape(renderer, VertexFormat::V2t222, PrimitiveType::TriFans,
                        "rounded_rectangle", "rounded_rectangle") {}

    /// Reserve memory for a number of rectangles.
    void reserve(size_t rectangles) { m_primitives.reserve(4 * rectangles); }

    /// Add new rounded rectangle.
    /// \param rect                 Position and size
    /// \param radius               Corner radius
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_rounded_rectangle(const FramebufferRect& rect, FramebufferPixels radius,
                               FramebufferPixels outline_thickness = 0);

    /// Add a rectangle slice.
    /// \param slice                Rectangle slice to draw within
    /// \param rect                 Rectangle position and size
    /// \param radius               Corner radius
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_rounded_rectangle_slice(const FramebufferRect& slice,
                                     const FramebufferRect& rect, FramebufferPixels radius,
                                     FramebufferPixels outline_thickness = 0);
};


/// A collection of rounded rectangle shapes.
/// Each rounded rectangle may have different size, color and outline thickness.
/// Antialiasing and softness is uniform.
class ColoredRoundedRectangle: public VaryingColorShape {
public:
    explicit ColoredRoundedRectangle(Renderer& renderer)
    : VaryingColorShape(renderer, VertexFormat::V2c44t222, PrimitiveType::TriFans,
                        "rounded_rectangle_c", "rounded_rectangle_c") {}

    /// Reserve memory for a number of rectangles.
    void reserve(size_t rectangles) { m_primitives.reserve(4 * rectangles); }

    /// Add new rectangle.
    /// \param rect                 Rectangle position and size.
    /// \param radius               Corner radius
    /// \param fill_color           Fill color
    /// \param outline_color        Outline color
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_rounded_rectangle(const FramebufferRect& rect, FramebufferPixels radius,
                               Color fill_color, Color outline_color = Color::Transparent(),
                               FramebufferPixels outline_thickness = 0);

    /// Add a rectangle slice. Can be used to draw partial outline.
    /// \param slice                Rectangle slice to draw within
    /// \param rect                 Rectangle position and size
    /// \param radius               Corner radius
    /// \param fill_color           Fill color
    /// \param outline_color        Outline color
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_rounded_rectangle_slice(const FramebufferRect& slice,
                                     const FramebufferRect& rect, FramebufferPixels radius,
                                     Color fill_color, Color outline_color,
                                     FramebufferPixels outline_thickness = 0);
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

    RoundedRectangleBuilder&
    add_rounded_rectangle_slice(const VariRect& slice, const VariRect& rect, VariUnits radius,
                                VariUnits outline_thickness = {})
    {
        m_shape.add_rounded_rectangle_slice(m_view.to_fb(slice), m_view.to_fb(rect), m_view.to_fb(radius),
                                            m_view.to_fb(outline_thickness));
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

    ColoredRoundedRectangleBuilder&
    add_rounded_rectangle_slice(const VariRect& slice, const VariRect& rect, VariUnits radius,
                                Color fill_color, Color outline_color, VariUnits outline_thickness = {})
    {
        m_shape.add_rounded_rectangle_slice(m_view.to_fb(slice), m_view.to_fb(rect), m_view.to_fb(radius),
                                            fill_color, outline_color, m_view.to_fb(outline_thickness));
        return *this;
    }

private:
    View& m_view;
    ColoredRoundedRectangle& m_shape;
};


} // namespace xci::graphics

#endif // XCI_GRAPHICS_SHAPE_ROUNDED_RECTANGLE_H
