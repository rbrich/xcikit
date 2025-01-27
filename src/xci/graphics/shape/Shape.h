// Shape.h created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_SHAPE_H
#define XCI_GRAPHICS_SHAPE_H

#include <xci/graphics/Primitives.h>
#include <xci/graphics/Renderer.h>
#include <xci/graphics/View.h>
#include <xci/graphics/Color.h>

namespace xci::graphics {


class Shape {
public:
    /// Remove all shapes in collection
    void clear() { m_primitives.clear(); }

    /// Draw all shapes to `view` at `pos`.
    /// Final shape position is `pos` + shapes' relative position
    void draw(View& view, VariCoords pos = {}) {
        if (!m_primitives.empty())
            m_primitives.draw(view, pos);
    }

protected:
    explicit Shape(Renderer& renderer,
                   VertexFormat vertex_format, PrimitiveType primitive_type,
                   std::string_view vert_shader, std::string_view frag_shader)
            : m_primitives(renderer, vertex_format, primitive_type),
              m_shader(renderer.get_shader(vert_shader, frag_shader)) {}

    Primitives m_primitives;
    Shader m_shader;
};


class UniformColorShape: public Shape {
public:
    /// Update GPU data (vertex buffers, uniforms etc.)
    /// \param fill_color       Set fill color for all shapes in the collection.
    /// \param outline_color    Set outline color for all shapes in the collection.
    /// \param softness         What fraction of outline should be smoothed (usable range is 0.0 - 1.0).
    ///                         This is extended "antialiasing" which mixes the outline color into fill color.
    /// \param antialiasing     How many fragments should be smoothed (usable range is 0.0 - 2.0).
    void update(Color fill_color = Color::Black(), Color outline_color = Color::White(),
                float softness = 0.0f, float antialiasing = 0.0f);

protected:
    explicit UniformColorShape(Renderer& renderer,
                               VertexFormat vertex_format, PrimitiveType primitive_type,
                               std::string_view vert_shader, std::string_view frag_shader)
    : Shape(renderer, vertex_format, primitive_type, vert_shader, frag_shader) {}
};


class VaryingColorShape: public Shape {
public:
    /// Update GPU data (vertex buffers, uniforms etc.)
    /// \param softness         What fraction of outline should be smoothed (usable range is 0.0 - 1.0).
    ///                         This is extended "antialiasing" which mixes the outline color into fill color.
    /// \param antialiasing     How many fragments should be smoothed (usable range is 0.0 - 2.0).
    void update(float softness = 0.0f, float antialiasing = 0.0f);

protected:
    explicit VaryingColorShape(Renderer& renderer,
                               VertexFormat vertex_format, PrimitiveType primitive_type,
                               std::string_view vert_shader, std::string_view frag_shader)
    : Shape(renderer, vertex_format, primitive_type, vert_shader, frag_shader) {}
};


/// Mixin class for setting uniform fill color, outline color
template <class T>
class UniformColorMixin {
public:
    /// Set fill color for all shapes in the collection
    T& set_fill_color(Color fill_color) { m_fill_color = fill_color; return static_cast<T&>(*this); }

    /// Set outline color for all shapes in the collection
    T& set_outline_color(Color outline_color) { m_outline_color = outline_color; return static_cast<T&>(*this); }

protected:
    Color m_fill_color = Color::Black();
    Color m_outline_color = Color::White();
};


/// Mixin class for setting uniform antialiasing and softness
template <class T>
class UniformAntialiasingMixin {
public:
    /// Set softness for all shapes in the collection
    /// This is extended "antialiasing" which mixes the outline color into fill color
    /// \param softness       What fraction of outline should be smoothed (usable range is 0.0 - 1.0)
    T& set_softness(float softness) { m_softness = softness; return static_cast<T&>(*this); }

    /// Set antialiasing for all shapes in the collection
    /// \param antialiasing   How many fragments should be smoothed (usable range is 0.0 - 2.0)
    T& set_antialiasing(float antialiasing) { m_antialiasing = antialiasing; return static_cast<T&>(*this);  }

protected:
    float m_softness = 0.0f;
    float m_antialiasing = 0.0f;
};


template <class T>
class UniformColorShapeBuilder: public UniformColorMixin<T>, public UniformAntialiasingMixin<T> {};

template <class T>
class VaryingColorShapeBuilder: public UniformAntialiasingMixin<T> {};


} // namespace xci::graphics

#endif // XCI_GRAPHICS_SHAPE_H
