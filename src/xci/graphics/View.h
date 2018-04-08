// View.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_VIEW_H
#define XCI_GRAPHICS_VIEW_H

#include <xci/util/geometry.h>

#include <memory>

namespace xci {
namespace graphics {

using xci::util::Vec2u;
using xci::util::Vec2f;


class View {
public:
    View();
    ~View();
    View(View&&) noexcept;
    View& operator=(View&&) noexcept;

    // Size of the view in screen pixels. This size might be different
    // from framebuffer size - in that case, call also `set_framebuffer_size`.
    void set_screen_size(Vec2u size);
    Vec2u screen_size() const;

    // Size of the view in framebuffer pixels.
    // This is used for pixel-perfect font rendering.
    // By default (or when set to {0, 0}, the framebuffer size will be set
    // to same value as view size in screen pixels.
    void set_framebuffer_size(Vec2u size);
    Vec2u framebuffer_size() const;

    // Size of the view in scalable units. These units are used
    // for placing objects in the view. The view size is at least 2 units
    // in either direction, with center at {0, 0} and aspect correction.
    // X goes right, Y goes down. Total size in one of the dimensions
    // will always equal 2.0.
    // Eg: {2.666, 2.0} for 800x600 (4/3 aspect ratio)
    Vec2f scalable_size() const;

    // Ratio of scalable units to screen pixels, ie.
    // size of 1x1 screen pixel in scalable units.
    Vec2f screen_ratio() const {
        auto s = scalable_size();
        auto px = screen_size();
        return {s.x / px.x, s.y / px.y};
    }

    // Ration of scalable units to framebuffer pixels, ie.
    // size of 1x1 framebuffer pixel in scalable units.
    Vec2f framebuffer_ratio() const {
        auto s = scalable_size();
        auto fb = framebuffer_size();
        return {s.x / fb.x, s.y / fb.y};
    }

    // ------------------------------------------------------------------------
    // Visual debugging

    using DebugFlags = unsigned int;
    enum class Debug: DebugFlags {
        GlyphBBox       = 1u << 0u,
        WordBBox        = 1u << 1u,
        WordBasePoint   = 1u << 2u,
        LineBBox        = 1u << 3u,
        SpanBBox        = 1u << 4u,
        PageBBox        = 1u << 5u,
    };
    void set_debug_flag(Debug flag, bool enabled = true);
    void set_debug_flags(DebugFlags flags) { m_debug = flags; }
    bool has_debug_flag(Debug flag) const;

    class Impl;
    Impl& impl() { return *m_impl; }

private:
    std::unique_ptr<Impl> m_impl;
    DebugFlags m_debug = 0;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_VIEW_H
