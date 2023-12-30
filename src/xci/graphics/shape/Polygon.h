// Polygon.h created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_SHAPE_POLYGON_H
#define XCI_GRAPHICS_SHAPE_POLYGON_H

#include "Shape.h"
#include <span>

namespace xci::graphics {


/// A collection of polygon shapes.
/// Each polygon may have different size and outline thickness.
/// Colors, antialiasing and softness are uniform.
class Polygon: public UniformColorShape {
public:
    explicit Polygon(Renderer& renderer)
    : UniformColorShape(renderer, VertexFormat::V2t3, PrimitiveType::TriFans,
                        "polygon", "polygon") {}

    /// Reserve memory for a number of polygon edges.
    /// E.g. five six-edged polygons => 30 edges
    void reserve(size_t polygon_edges) { m_primitives.reserve(polygon_edges); }

    /// Add a polygon.
    /// It can be regular or irregular, convex or concave. The only limitation is that
    /// that each vertex must be directly visible from the center, without crossing any edges.
    /// \param center       Invisible center of TriFan structure.
    ///                     Triangles are constructed from center to each edge.
    /// \param vertices     Edge vertices around center, in CCW order.
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_polygon(FramebufferCoords center, std::span<const FramebufferCoords> vertices,
                     FramebufferPixels outline_thickness = 0);
};


/// A collection of polygon shapes.
/// Each polygon may have different size, color and outline thickness.
/// Antialiasing and softness is uniform.
class ColoredPolygon: public VaryingColorShape {
public:
    explicit ColoredPolygon(Renderer& renderer)
    : VaryingColorShape(renderer, VertexFormat::V2c44t3, PrimitiveType::TriFans,
                        "polygon_c", "polygon_c") {}

    /// Reserve memory for a number of polygon edges.
    /// E.g. five six-edged polygons => 30 edges
    void reserve(size_t polygon_edges) { m_primitives.reserve(polygon_edges); }

    /// Add a polygon.
    /// It can be regular or irregular, convex or concave. The only limitation is that
    /// that each vertex must be directly visible from the center, without crossing any edges.
    /// \param center       Invisible center of TriFan structure.
    ///                     Triangles are constructed from center to each edge.
    /// \param vertices     Edge vertices around center, in CCW order.
    /// \param fill_color           Fill color
    /// \param outline_color        Outline color
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_polygon(FramebufferCoords center, std::span<const FramebufferCoords> vertices,
                     Color fill_color, Color outline_color, FramebufferPixels outline_thickness);
};


/// Convenience - build polygons in resize() method with any units
class PolygonBuilder: public UniformColorShapeBuilder<PolygonBuilder> {
public:
    PolygonBuilder(View& view, Polygon& polygon) : m_view(view), m_polygon(polygon) { m_polygon.clear(); }
    ~PolygonBuilder() { m_polygon.update(m_fill_color, m_outline_color, m_softness, m_antialiasing); }

    PolygonBuilder& begin_polygon(VariCoords center) {
        m_center = m_view.to_fb(center);
        m_vertices.clear();
        return *this;
    }

    PolygonBuilder& add_vertex(VariCoords vertex) {
        m_vertices.push_back(m_view.to_fb(vertex));
        return *this;
    }

    PolygonBuilder& end_polygon(VariUnits outline_thickness = {}) {
        m_polygon.add_polygon(m_center, m_vertices, m_view.to_fb(outline_thickness));
        return *this;
    }

private:
    View& m_view;
    Polygon& m_polygon;
    FramebufferCoords m_center;
    std::vector<FramebufferCoords> m_vertices;
};


/// Convenience - build colored polygons in resize() method with any units
class ColoredPolygonBuilder: public VaryingColorShapeBuilder<ColoredPolygonBuilder> {
public:
    ColoredPolygonBuilder(View& view, ColoredPolygon& polygon) : m_view(view), m_polygon(polygon) { m_polygon.clear(); }
    ~ColoredPolygonBuilder() { m_polygon.update(m_softness, m_antialiasing); }

    ColoredPolygonBuilder& begin_polygon(VariCoords center) {
        m_center = m_view.to_fb(center);
        m_vertices.clear();
        return *this;
    }

    ColoredPolygonBuilder& add_vertex(VariCoords vertex) {
        m_vertices.push_back(m_view.to_fb(vertex));
        return *this;
    }

    ColoredPolygonBuilder& end_polygon(Color fill_color, Color outline_color, VariUnits outline_thickness = {}) {
        m_polygon.add_polygon(m_center, m_vertices, fill_color, outline_color, m_view.to_fb(outline_thickness));
        return *this;
    }

private:
    View& m_view;
    ColoredPolygon& m_polygon;
    FramebufferCoords m_center;
    std::vector<FramebufferCoords> m_vertices;
};


} // namespace xci::graphics

#endif // XCI_GRAPHICS_SHAPE_POLYGON_H
