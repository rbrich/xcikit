// Line.h created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_SHAPE_LINE_H
#define XCI_GRAPHICS_SHAPE_LINE_H

#include "Shape.h"

namespace xci::graphics {


/// A collection of line shapes.
/// Each line may have different size and thickness.
/// Colors, antialiasing and softness are uniform.
class Line: public UniformColorShape {
public:
    explicit Line(Renderer& renderer)
    : UniformColorShape(renderer, VertexFormat::V2t2, PrimitiveType::TriFans, ShaderId::Line) {}

    /// Reserve memory for a number of lines.
    void reserve(size_t lines) { m_primitives.reserve(4 * lines); }

    /// Add a line segment
    /// \param a, b         Two points to define the line
    /// \param thickness    Line width, measured perpendicularly from a-b
    void add_line(FramebufferCoords a, FramebufferCoords b,
                  FramebufferPixels thickness);

    /// Add a slice of infinite line
    /// \param a, b         Two points to define the line
    /// \param slice        Rectangular region in which the line is visible
    /// \param thickness    Line width, measured perpendicularly from a-b
    ///
    ///   ---- a --- b ----
    ///                    > thickness
    ///   -----------------
    void add_line_slice(const FramebufferRect& slice,
                        FramebufferCoords a, FramebufferCoords b,
                        FramebufferPixels thickness);
};


/// A collection of line shapes.
/// Each line may have different size, color and thickness.
/// Antialiasing and softness is uniform.
class ColoredLine: public VaryingColorShape {
public:
    explicit ColoredLine(Renderer& renderer)
    : VaryingColorShape(renderer, VertexFormat::V2c44t22, PrimitiveType::TriFans, ShaderId::LineC) {}

    /// Reserve memory for a number of lines.
    void reserve(size_t lines) { m_primitives.reserve(4 * lines); }

    /// Add a line segment
    /// \param a, b         Two points to define the line
    /// \param fill_color           Fill color
    /// \param outline_color        Outline color
    /// \param thickness    Line width, measured perpendicularly from a-b
    void add_line(FramebufferCoords a, FramebufferCoords b,
                  Color fill_color, Color outline_color, FramebufferPixels thickness);

    /// Add a slice of infinite line
    /// \param a, b         Two points to define the line
    /// \param slice        Rectangular region in which the line is visible
    /// \param fill_color           Fill color
    /// \param outline_color        Outline color
    /// \param thickness    Line width, measured perpendicularly from a-b
    ///
    ///   ---- a --- b ----
    ///                    > thickness
    ///   -----------------
    void add_line_slice(const FramebufferRect& slice,
                        FramebufferCoords a, FramebufferCoords b,
                        Color fill_color, Color outline_color, FramebufferPixels thickness);
};


/// Convenience - build lines in resize() method with any units
class LineBuilder: public UniformColorShapeBuilder<LineBuilder> {
public:
    LineBuilder(View& view, Line& line) : m_view(view), m_line(line) { m_line.clear(); }
    ~LineBuilder() { m_line.update(m_fill_color, m_outline_color, m_softness, m_antialiasing); }

    LineBuilder&
    add_line(VariCoords a, VariCoords b, VariUnits thickness) {
        m_line.add_line(m_view.to_fb(a), m_view.to_fb(b), m_view.to_fb(thickness));
        return *this;
    }

    LineBuilder&
    add_line_slice(const VariRect& slice, VariCoords a, VariCoords b, VariUnits thickness) {
        m_line.add_line_slice(m_view.to_fb(slice), m_view.to_fb(a), m_view.to_fb(b), m_view.to_fb(thickness));
        return *this;
    }

private:
    View& m_view;
    Line& m_line;
};


/// Convenience - build colored lines in resize() method with any units
class ColoredLineBuilder: public VaryingColorShapeBuilder<ColoredLineBuilder> {
public:
    ColoredLineBuilder(View& view, ColoredLine& line) : m_view(view), m_line(line) { m_line.clear(); }
    ~ColoredLineBuilder() { m_line.update(m_softness, m_antialiasing); }

    ColoredLineBuilder&
    add_line(VariCoords a, VariCoords b, Color fill_color, Color outline_color, VariUnits thickness) {
        m_line.add_line(m_view.to_fb(a), m_view.to_fb(b), fill_color, outline_color, m_view.to_fb(thickness));
        return *this;
    }

    ColoredLineBuilder&
    add_line_slice(const VariRect& slice, VariCoords a, VariCoords b,
                   Color fill_color, Color outline_color, VariUnits thickness)
    {
        m_line.add_line_slice(m_view.to_fb(slice), m_view.to_fb(a), m_view.to_fb(b),
                              fill_color, outline_color, m_view.to_fb(thickness));
        return *this;
    }

private:
    View& m_view;
    ColoredLine& m_line;
};


} // namespace xci::graphics

#endif // XCI_GRAPHICS_SHAPE_LINE_H
