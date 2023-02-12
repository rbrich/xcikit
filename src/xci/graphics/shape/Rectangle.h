// Rectangle.h created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_SHAPE_RECTANGLE_H
#define XCI_GRAPHICS_SHAPE_RECTANGLE_H

#include "Shape.h"

namespace xci::graphics {


/// A collection of rectangle shapes.
/// Each rectangle may have different size and outline thickness.
/// Colors, antialiasing and softness are uniform.
class Rectangle: public UniformColorShape {
public:
    explicit Rectangle(Renderer& renderer)
    : UniformColorShape(renderer, VertexFormat::V2t22, PrimitiveType::TriFans, ShaderId::Rectangle) {}

    /// Reserve memory for a number of rectangles.
    void reserve(size_t rectangles) { m_primitives.reserve(4 * rectangles); }

    /// Add new rectangle.
    /// \param rect                 Rectangle position and size.
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_rectangle(const FramebufferRect& rect,
                       FramebufferPixels outline_thickness = 0);

    /// Add a rectangle slice.
    /// \param slice                Rectangle slice to draw within
    /// \param rect                 Rectangle position and size
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_rectangle_slice(const FramebufferRect& slice, const FramebufferRect& rect,
                             FramebufferPixels outline_thickness = 0);
};


/// A collection of rectangle shapes.
/// Each rectangle may have different size, color and outline thickness.
/// Antialiasing and softness is uniform.
class ColoredRectangle: public VaryingColorShape {
public:
    explicit ColoredRectangle(Renderer& renderer)
    : VaryingColorShape(renderer, VertexFormat::V2c44t22, PrimitiveType::TriFans, ShaderId::RectangleC) {}

    /// Reserve memory for a number of rectangles.
    void reserve(size_t rectangles) { m_primitives.reserve(4 * rectangles); }

    /// Add new rectangle.
    /// \param rect                 Rectangle position and size.
    /// \param fill_color           Fill color
    /// \param outline_color        Outline color
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_rectangle(const FramebufferRect& rect,
                       Color fill_color, Color outline_color = Color::Transparent(),
                       FramebufferPixels outline_thickness = 0);

    /// Add a rectangle slice. Can be used to draw partial outline.
    /// \param slice                Rectangle slice to draw within
    /// \param rect                 Rectangle position and size
    /// \param fill_color           Fill color
    /// \param outline_color        Outline color
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_rectangle_slice(const FramebufferRect& slice, const FramebufferRect& rect,
                             Color fill_color, Color outline_color,
                             FramebufferPixels outline_thickness = 0);
};


/// Convenience - build rectangles in resize() method with any units
class RectangleBuilder: public UniformColorShapeBuilder<RectangleBuilder> {
public:
    RectangleBuilder(View& view, Rectangle& rectangle) : m_view(view), m_rectangle(rectangle) { m_rectangle.clear(); }
    ~RectangleBuilder() { m_rectangle.update(m_fill_color, m_outline_color, m_softness, m_antialiasing); }

    RectangleBuilder&
    add_rectangle(const VariRect& rect, VariUnits outline_thickness = {})
    {
        m_rectangle.add_rectangle(m_view.to_fb(rect), m_view.to_fb(outline_thickness));
        return *this;
    }

    RectangleBuilder&
    add_rectangle_slice(const VariRect& slice, const VariRect& rect,
                        VariUnits outline_thickness = {})
    {
        m_rectangle.add_rectangle_slice(m_view.to_fb(slice), m_view.to_fb(rect),
                                        m_view.to_fb(outline_thickness));
        return *this;
    }

private:
    View& m_view;
    Rectangle& m_rectangle;
};


/// Convenience - build colored rectangles in resize() method with any units
class ColoredRectangleBuilder: public VaryingColorShapeBuilder<ColoredRectangleBuilder> {
public:
    ColoredRectangleBuilder(View& view, ColoredRectangle& rectangle) : m_view(view), m_rectangle(rectangle) { m_rectangle.clear(); }
    ~ColoredRectangleBuilder() { m_rectangle.update(m_softness, m_antialiasing); }

    ColoredRectangleBuilder&
    add_rectangle(const VariRect& rect,
                  Color fill_color, Color outline_color, VariUnits outline_thickness = {})
    {
        m_rectangle.add_rectangle(m_view.to_fb(rect),
                                  fill_color, outline_color, m_view.to_fb(outline_thickness));
        return *this;
    }

    ColoredRectangleBuilder&
    add_rectangle_slice(const VariRect& slice, const VariRect& rect,
                        Color fill_color, Color outline_color, VariUnits outline_thickness = {})
    {
        m_rectangle.add_rectangle_slice(m_view.to_fb(slice), m_view.to_fb(rect),
                                        fill_color, outline_color, m_view.to_fb(outline_thickness));
        return *this;
    }

private:
    View& m_view;
    ColoredRectangle& m_rectangle;
};


} // namespace xci::graphics

#endif // XCI_GRAPHICS_SHAPE_RECTANGLE_H
