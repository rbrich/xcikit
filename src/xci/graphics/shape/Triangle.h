// Triangle.h created on 2023-02-25 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_SHAPE_TRIANGLE_H
#define XCI_GRAPHICS_SHAPE_TRIANGLE_H

#include "Shape.h"

namespace xci::graphics {


/// A collection of triangle shapes.
/// Each triangle may have different size and outline thickness.
/// Colors, antialiasing and softness are uniform.
class Triangle: public UniformColorShape {
public:
    explicit Triangle(Renderer& renderer)
    : UniformColorShape(renderer, VertexFormat::V2t3, PrimitiveType::TriFans, ShaderId::Triangle) {}

    /// Reserve memory for a number of triangles.
    void reserve(size_t triangles) { m_primitives.reserve(3 * triangles); }

    /// Add a triangle.
    /// \param v1, v2, v3           Vertices in CCW order.
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_triangle(FramebufferCoords v1, FramebufferCoords v2, FramebufferCoords v3,
                      FramebufferPixels outline_thickness = 0);
};


/// A collection of triangle shapes.
/// Each triangle may have different size, color and outline thickness.
/// Antialiasing and softness is uniform.
class ColoredTriangle: public VaryingColorShape {
public:
    explicit ColoredTriangle(Renderer& renderer)
    : VaryingColorShape(renderer, VertexFormat::V2c44t3, PrimitiveType::TriFans, ShaderId::TriangleC) {}

    /// Reserve memory for a number of triangle edges.
    /// E.g. five six-edged triangles => 30 edges
    void reserve(size_t triangles) { m_primitives.reserve(3 * triangles); }

    /// Add a triangle.
    /// \param v1, v2, v3           Vertices in CCW order.
    /// \param fill_color           Fill color
    /// \param outline_color        Outline color
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_triangle(FramebufferCoords v1, FramebufferCoords v2, FramebufferCoords v3,
                      Color fill_color, Color outline_color, FramebufferPixels outline_thickness);
};


/// Convenience - build triangles in resize() method with any units
class TriangleBuilder: public UniformColorShapeBuilder<TriangleBuilder> {
public:
    TriangleBuilder(View& view, Triangle& triangle) : m_view(view), m_triangle(triangle) { m_triangle.clear(); }
    ~TriangleBuilder() { m_triangle.update(m_fill_color, m_outline_color, m_softness, m_antialiasing); }

    TriangleBuilder&
    add_triangle(VariCoords v1, VariCoords v2, VariCoords v3, VariUnits outline_thickness = {})
    {
        m_triangle.add_triangle(m_view.to_fb(v1), m_view.to_fb(v2), m_view.to_fb(v3),
                                m_view.to_fb(outline_thickness));
        return *this;
    }

private:
    View& m_view;
    Triangle& m_triangle;
};


/// Convenience - build colored triangles in resize() method with any units
class ColoredTriangleBuilder: public VaryingColorShapeBuilder<ColoredTriangleBuilder> {
public:
    ColoredTriangleBuilder(View& view, ColoredTriangle& triangle) : m_view(view), m_triangle(triangle) { m_triangle.clear(); }
    ~ColoredTriangleBuilder() { m_triangle.update(m_softness, m_antialiasing); }

    ColoredTriangleBuilder&
    add_triangle(VariCoords v1, VariCoords v2, VariCoords v3,
                 Color fill_color, Color outline_color, VariUnits outline_thickness = {})
    {
        m_triangle.add_triangle(m_view.to_fb(v1), m_view.to_fb(v2), m_view.to_fb(v3),
                                fill_color, outline_color, m_view.to_fb(outline_thickness));
        return *this;
    }

private:
    View& m_view;
    ColoredTriangle& m_triangle;
};


} // namespace xci::graphics

#endif // XCI_GRAPHICS_SHAPE_TRIANGLE_H
