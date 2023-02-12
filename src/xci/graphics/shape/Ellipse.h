// Ellipse.h created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_SHAPE_ELLIPSE_H
#define XCI_GRAPHICS_SHAPE_ELLIPSE_H

#include "Shape.h"

namespace xci::graphics {


/// A collection of ellipse shapes.
/// Each ellipse may have different size and outline thickness.
/// Colors, antialiasing and softness are uniform.
class Ellipse: public UniformColorShape {
public:
    explicit Ellipse(Renderer& renderer)
    : UniformColorShape(renderer, VertexFormat::V2t22, PrimitiveType::TriFans, ShaderId::Ellipse) {}

    /// Reserve memory for a number of ellipses
    void reserve(size_t ellipses) { m_primitives.reserve(4 * ellipses); }

    /// Add new ellipse.
    /// \param rect                 Ellipse position and size
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_ellipse(const FramebufferRect& rect,
                     FramebufferPixels outline_thickness = 0);

    /// Add ellipse slice.
    /// \param slice                Rectangle slice to draw within
    /// \param ellipse              Ellipse position and size
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_ellipse_slice(const FramebufferRect& slice, const FramebufferRect& ellipse,
                           FramebufferPixels outline_thickness = 0);

    /// Add new circle.
    /// \param center               A point the circle has center
    /// \param radius               Radius of the circle
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_circle(FramebufferCoords center, FramebufferPixels radius,
                    FramebufferPixels outline_thickness = 0)
    {
        add_ellipse({center.x - radius, center.y - radius, 2 * radius, 2 * radius},
                    outline_thickness);
    }
};


/// A collection of ellipse shapes.
/// Each ellipse may have different size, color and outline thickness.
/// Antialiasing and softness is uniform.
class ColoredEllipse: public VaryingColorShape {
public:
    explicit ColoredEllipse(Renderer& renderer)
    : VaryingColorShape(renderer, VertexFormat::V2c44t22, PrimitiveType::TriFans, ShaderId::EllipseC) {}

    /// Reserve memory for a number of ellipses
    void reserve(size_t ellipses) { m_primitives.reserve(4 * ellipses); }

    /// Add new ellipse.
    /// \param rect                 Ellipse position and size
    /// \param fill_color           Fill color
    /// \param outline_color        Outline color
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_ellipse(const FramebufferRect& rect,
                     Color fill_color, Color outline_color, FramebufferPixels outline_thickness = 0);

    /// Add ellipse slice.
    /// \param slice                Rectangle slice to draw within
    /// \param ellipse              Ellipse position and size
    /// \param fill_color           Fill color
    /// \param outline_color        Outline color
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_ellipse_slice(const FramebufferRect& slice, const FramebufferRect& ellipse,
                           Color fill_color, Color outline_color, FramebufferPixels outline_thickness = 0);

    /// Add new circle.
    /// \param center               A point the circle has center
    /// \param radius               Radius of the circle
    /// \param fill_color           Fill color
    /// \param outline_color        Outline color
    /// \param outline_thickness    The outline goes from edge to inside.
    ///                             This parameter defines how far (in framebuffer pixels).
    void add_circle(FramebufferCoords center, FramebufferPixels radius,
                    Color fill_color, Color outline_color, FramebufferPixels outline_thickness = 0)
    {
        add_ellipse({center.x - radius, center.y - radius, 2 * radius, 2 * radius},
                    fill_color, outline_color, outline_thickness);
    }
};


/// Convenience - build ellipses in resize() method with any units
class EllipseBuilder: public UniformColorShapeBuilder<EllipseBuilder> {
public:
    EllipseBuilder(View& view, Ellipse& ellipse)
            : m_view(view), m_ellipse(ellipse) { m_ellipse.clear(); }
    ~EllipseBuilder() { m_ellipse.update(m_fill_color, m_outline_color, m_softness, m_antialiasing); }

    EllipseBuilder&
    add_ellipse(const VariRect& rect, VariUnits outline_thickness = {})
    {
        m_ellipse.add_ellipse(m_view.to_fb(rect), m_view.to_fb(outline_thickness));
        return *this;
    }

    EllipseBuilder&
    add_ellipse_slice(const VariRect& slice, const VariRect& ellipse,
                      VariUnits outline_thickness = {})
    {
        m_ellipse.add_ellipse_slice(m_view.to_fb(slice), m_view.to_fb(ellipse),
                                    m_view.to_fb(outline_thickness));
        return *this;
    }

    EllipseBuilder&
    add_circle(VariCoords center, VariUnits radius,
               VariUnits outline_thickness = {})
    {
        m_ellipse.add_circle(m_view.to_fb(center), m_view.to_fb(radius),
                             m_view.to_fb(outline_thickness));
        return *this;
    }

private:
    View& m_view;
    Ellipse& m_ellipse;
};


/// Convenience - build colored ellipses in resize() method with any units
class ColoredEllipseBuilder: public VaryingColorShapeBuilder<EllipseBuilder> {
public:
    ColoredEllipseBuilder(View& view, ColoredEllipse& ellipse)
            : m_view(view), m_ellipse(ellipse) { m_ellipse.clear(); }
    ~ColoredEllipseBuilder() { m_ellipse.update(m_softness, m_antialiasing); }

    ColoredEllipseBuilder&
    add_ellipse(const VariRect& rect,
                Color fill_color, Color outline_color, VariUnits outline_thickness = {})
    {
        m_ellipse.add_ellipse(m_view.to_fb(rect),
                              fill_color, outline_color, m_view.to_fb(outline_thickness));
        return *this;
    }

    ColoredEllipseBuilder&
    add_ellipse_slice(const VariRect& slice, const VariRect& ellipse,
                      Color fill_color, Color outline_color, VariUnits outline_thickness = {})
    {
        m_ellipse.add_ellipse_slice(m_view.to_fb(slice), m_view.to_fb(ellipse),
                                    fill_color, outline_color, m_view.to_fb(outline_thickness));
        return *this;
    }

    ColoredEllipseBuilder&
    add_circle(VariCoords center, VariUnits radius,
               Color fill_color, Color outline_color, VariUnits outline_thickness = {})
    {
        m_ellipse.add_circle(m_view.to_fb(center), m_view.to_fb(radius),
                             fill_color, outline_color, m_view.to_fb(outline_thickness));
        return *this;
    }

private:
    View& m_view;
    ColoredEllipse& m_ellipse;
};


} // namespace xci::graphics

#endif // XCI_GRAPHICS_SHAPE_ELLIPSE_H
