// Shape.h created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_SHAPES_H
#define XCI_GRAPHICS_SHAPES_H

#include "Primitives.h"
#include "Renderer.h"
#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>
#include <xci/core/geometry.h>
#include <xci/core/mixin.h>

namespace xci::graphics {

using xci::core::Rect_f;
using xci::core::Vec2f;


/// A collection of one of basic shapes: rectangles, ellipses, lines.
/// Each shape may have different size and outline width,
/// but colors are uniform.
class Shape: private core::NonCopyable {
public:
    explicit Shape(Renderer& renderer,
                   Color fill_color = Color::Black(),
                   Color outline_color = Color::White());

    void set_fill_color(Color fill_color) { m_fill_color = fill_color; }
    void set_outline_color(Color outline_color) { m_outline_color = outline_color; }
    void set_antialiasing(float antialiasing) { m_antialiasing = antialiasing; }
    void set_softness(float softness) { m_softness = softness; }

    Color fill_color() const { return m_fill_color; }
    Color outline_color() const { return m_outline_color; }

    /// Add a line segment
    /// \param a, b         Two points to define the line
    /// \param thickness    Line width, measured perpendicularly from a-b
    void add_line(FramebufferCoords a, FramebufferCoords b,
                  FramebufferPixels thickness);

    /// Add a slice of infinite line
    /// \param a, b         Two points to define the line
    /// \param slice`       Rectangular region in which the line is visible
    /// \param thickness    Line width, measured perpendicularly from a-b
    ///
    ///   ---- a --- b ----
    ///                    > thickness
    ///   -----------------
    void add_line_slice(const FramebufferRect& slice,
                        FramebufferCoords a, FramebufferCoords b,
                        FramebufferPixels thickness);

    /// Add new rectangle.
    /// \param rect                 Rectangle position and size.
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_rectangle(const FramebufferRect& rect,
                       FramebufferPixels outline_thickness = 0);
    void add_rectangle_slice(const FramebufferRect& slice, const FramebufferRect& rect,
                             FramebufferPixels outline_thickness = 0);

    /// Add new ellipse.
    /// \param rect                 Ellipse position and size
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_ellipse(const FramebufferRect& rect,
                     FramebufferPixels outline_thickness = 0);
    void add_ellipse_slice(const FramebufferRect& slice, const FramebufferRect& ellipse,
                           FramebufferPixels outline_thickness = 0);

    /// Add new circle.
    /// \param center               A point the circle has center
    /// \param radius               Radius of the circle
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_circle(FramebufferCoords center, float radius,
                    FramebufferPixels outline_thickness = 0);

    /// Add new rounded rectangle.
    /// \param rect                 Position and size
    /// \param radius               Corner radius
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in display units).
    void add_rounded_rectangle(const FramebufferRect& rect, FramebufferPixels radius,
                               FramebufferPixels outline_thickness = 0);

    /// Reserve memory for a number of `lines`, `rectangles`, `ellipses`.
    void reserve(size_t lines, size_t rectangles, size_t ellipses);

    /// Remove all shapes and clear all state (colors etc.)
    void clear();

    /// Update shapes attributes according to settings (color etc.)
    void update();

    /// Draw all shapes to `view` at `pos`.
    /// Final shape position is `pos` + shapes' relative position
    void draw(View& view, VariCoords pos);

private:
    Color m_fill_color;
    Color m_outline_color;
    float m_antialiasing = 0;
    float m_softness = 0;

    Primitives m_lines;
    Primitives m_rectangles;
    Primitives m_ellipses;

    Shader& m_line_shader;
    Shader& m_rectangle_shader;
    Shader& m_ellipse_shader;
};


/// Convenience - build shapes in resize() method with any units
class ShapeBuilder {
public:
    ShapeBuilder(View& view, Shape& shape) : m_view(view), m_shape(shape) { m_shape.clear(); }
    ~ShapeBuilder() { m_shape.update(); }

    ShapeBuilder& add_line_slice(const VariRect& slice, VariCoords a, VariCoords b, VariUnits thickness);
    ShapeBuilder& add_rectangle(const VariRect& rect, VariUnits outline_thickness = {});
    ShapeBuilder& add_rectangle_slice(const VariRect& slice, const VariRect& rect, VariUnits outline_thickness = {});
    ShapeBuilder& add_ellipse(const VariRect& rect, VariUnits outline_thickness = {});
    ShapeBuilder& add_ellipse_slice(const VariRect& slice, const VariRect& ellipse, VariUnits outline_thickness = {});
    ShapeBuilder& add_rounded_rectangle(const VariRect& rect, VariUnits radius, VariUnits outline_thickness = {});

private:
    View& m_view;
    Shape& m_shape;
};

} // namespace xci::graphics

#endif //XCI_GRAPHICS_SHAPES_H
