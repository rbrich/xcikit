// View.h created on 2018-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_VIEW_H
#define XCI_GRAPHICS_VIEW_H

#include <xci/core/geometry.h>
#include <xci/compat/macros.h>

#include <fmt/ostream.h>

#include <memory>
#include <vector>
#include <array>
#include <cassert>

namespace xci::graphics {


enum class Unit {
    FramebufferPixel,   // actual GPU pixels
    ScreenPixel,        // virtual screen pixels
    ViewportUnit,       // relative units derived from viewport size and aspect ratio (see below)
};

template <typename T, Unit u>
struct Units {
    constexpr Units() = default;
    constexpr Units(T value) : value(value) {}

    bool operator <(Units rhs) const { return value < rhs.value; }
    bool operator >(Units rhs) const { return value > rhs.value; }
    bool operator <=(Units rhs) const { return value <= rhs.value; }
    bool operator >=(Units rhs) const { return value >= rhs.value; }
    bool operator ==(Units rhs) const { return value == rhs.value; }
    bool operator !=(Units rhs) const { return value != rhs.value; }

    Units operator -() const { return -value; }
    Units operator +=(Units rhs) { value += rhs.value; return value; }
    Units operator -=(Units rhs) { value -= rhs.value; return value; }
    friend Units operator *(Units lhs, Units rhs) { return lhs.value * rhs.value; }
    friend Units operator /(Units lhs, Units rhs) { return lhs.value / rhs.value; }
    friend Units operator +(Units lhs, Units rhs) { return lhs.value + rhs.value; }
    friend Units operator -(Units lhs, Units rhs) { return lhs.value - rhs.value; }
    friend std::ostream& operator <<(std::ostream& s, Units rhs) { return s << rhs.value; }

    constexpr Unit unit() const { return u; }

    template <typename U>
    U as() const { return static_cast<U>(value); }

    // Value of Units can be explicitly cast to T
    using numeric_type = T;
    constexpr explicit operator T() const { return value; }

    T value = 0;
};


using FramebufferPixels = Units<float, Unit::FramebufferPixel>;
using FramebufferCoords = core::Vec2<FramebufferPixels>;
using FramebufferSize = FramebufferCoords;
using FramebufferRect = core::Rect<FramebufferPixels>;

using ScreenPixels = Units<float, Unit::ScreenPixel>;
using ScreenCoords = core::Vec2<ScreenPixels>;
using ScreenSize = ScreenCoords;
using ScreenRect = core::Rect<ScreenPixels>;

using ViewportUnits = Units<float, Unit::ViewportUnit>;
using ViewportCoords = core::Vec2<ViewportUnits>;
using ViewportSize = ViewportCoords;
using ViewportRect = core::Rect<ViewportUnits>;

namespace unit_literals {
constexpr FramebufferPixels operator ""_fb (long double value) { return {float(value)}; }
constexpr FramebufferPixels operator ""_fb (unsigned long long value) { return {float(value)}; }
constexpr ScreenPixels operator ""_px (long double value) { return {float(value)}; }
constexpr ScreenPixels operator ""_px (unsigned long long value) { return {float(value)}; }
constexpr ViewportUnits operator ""_vp (long double value) { return {float(value)}; }
constexpr ViewportUnits operator ""_vp (unsigned long long value) { return {float(value)}; }
} // namespace unit_literals


/// A variant which can contain FramebufferPixels, ScreenPixels, ViewportUnits
/// Optimized for size - it's 4 bytes, same as the original types.
/// The pixels units do not lose any precision. ViewportUnits is stored in fixed point format.
/// FramebufferPixels - encoded in range -536870912 .. 536870911 (three upper bits are 000 or 111)
///                   - the value is fixed point decimal in 19 dot 10 bits format (+ 1 bit sign)
/// ScreenPixels     - encoded in range -1073741824 .. 1073741823 (three upper bits are 001 or 110)
///                  - xor 0x20000000 to decode original value
///                  - the value is fixed point decimal in 19 dot 10 bits format (+ 1 bit sign)
/// ViewportUnits    - encoded in range -2147483648 .. 2147483647 (two upper bits are 01 or 10)
///                  - xor 0x40000000 to decode original value (30 bits + sign)
///                  - the value is fixed point decimal in 14 dot 16 bits format (+ 1 bit sign)
class VariUnits {
public:
    VariUnits() = default;
    VariUnits(FramebufferPixels fb) : m_storage(to_storage(fb)) {}
    VariUnits(ScreenPixels px) : m_storage(to_storage(px)) {}
    VariUnits(ViewportUnits vp) : m_storage(to_storage(vp)) {}

    // Get contained type
    enum Type { Framebuffer, Screen, Viewport };
    Type type() const;

    // Get contained value
    // Unchecked (assert in debug). Returns garbage when asked for different type than contained.
    FramebufferPixels as_framebuffer() const;
    ScreenPixels as_screen() const;
    ViewportUnits as_viewport() const;

    // For tests
    int32_t raw_storage() const { return m_storage; }

    // Limited operations directly on VariUnits.
    // Cannot overload operators, because we have implicit ctor from actual units.
    VariUnits mul(float v);

    operator bool() const { return m_storage != 0; }
    friend std::ostream& operator <<(std::ostream& s, VariUnits rhs);

private:
    static int32_t to_storage(FramebufferPixels fb);
    static int32_t to_storage(ScreenPixels px);
    static int32_t to_storage(ViewportUnits vp);

    int32_t m_storage = 0;
};

struct VariCoords: public core::Vec2<VariUnits> {
    VariCoords() = default;
    VariCoords(core::Vec2<VariUnits> v) : core::Vec2<VariUnits>(v) {}
    VariCoords(FramebufferCoords v) : core::Vec2<VariUnits>(v.x, v.y) {}
    VariCoords(ScreenCoords v) : core::Vec2<VariUnits>(v.x, v.y) {}
    VariCoords(ViewportCoords v) : core::Vec2<VariUnits>(v.x, v.y) {}
    VariCoords(VariUnits x, VariUnits y) : core::Vec2<VariUnits>(x, y) {}
};

using VariSize = VariCoords;
using VariRect = core::Rect<VariUnits>;


enum class ViewOrigin {
    Center,
    TopLeft,
};


class Window;

class View {
public:
    View() = default;
    explicit View(Window* m_window) : m_window(m_window) {}

    // Window might be nullptr if the View was created directly
    Window* window() const { return m_window; }

    // Compute projection matrix based on viewport size and offset
    std::array<float, 16> projection_matrix() const;

    // ------------------------------------------------------------------------
    // Sizes, coordinates

    /// Set origin of the coordinates (0,0).
    /// This affects all units (framebuffer, screen, viewport).
    /// Default: Center
    void set_origin(ViewOrigin origin);

    // Size of the view in screen pixels. This size might be different
    // from framebuffer size - in that case, call also `set_framebuffer_size`.
    bool set_screen_size(ScreenSize size);
    ScreenSize screen_size() const { return m_screen_size; }
    ScreenCoords screen_center() const;
    ScreenCoords screen_top_left(ScreenCoords offset = {}) const;

    // Size of the view in framebuffer pixels.
    // This is used for pixel-perfect font rendering.
    // By default, (or when set to {0, 0}), the framebuffer size will be set
    // to same value as view size in screen pixels.
    bool set_framebuffer_size(FramebufferSize size);
    FramebufferSize framebuffer_size() const { return m_framebuffer_size; }

    /// Viewport center in framebuffer coordinates.
    /// Start at these coordinates to center things in the viewport.
    FramebufferCoords framebuffer_center() const;

    /// Viewport origin in real (underlying) framebuffer coordinates.
    /// Add this to FramebufferCoords to translate them to underlying VkViewport coords.
    /// In reverse, subtract from reported (e.g. mouse) underlying coords to translate them to view.
    FramebufferCoords framebuffer_origin() const;

    /// The viewport units are based on smaller viewport dimension.
    /// One unit is by default 1% of the viewport. The default scale is 100.
    /// Using this method, the scale can be changed to dynamically enlarge/shrink
    /// the whole UI, when it's based on the viewport units.
    void set_viewport_scale(float scale);
    float viewport_scale() const { return m_vp_scale; }

    /// Viewport size in viewport units.
    /// E.g: {133.33, 100.0} for 800x600 (4/3 aspect ratio)
    ViewportSize viewport_size() const { return m_viewport_size; }

    /// Coordinates of viewport center in viewport units.
    /// By default {0, 0}. With TopLeft origin, it will be e.g. {66.66, 50}
    ViewportCoords viewport_center() const;
    ViewportCoords viewport_top_left(ViewportCoords offset = {}) const;

    // Convert units to framebuffer / screen:

    FramebufferPixels px_to_fb(ScreenPixels value) const {
        return value.value * framebuffer_size().y.value / screen_size().y.value;
    }

    FramebufferPixels vp_to_fb(ViewportUnits value) const {
        return value.value * framebuffer_size().y.value / viewport_size().y.value;
    }

    FramebufferPixels to_fb(VariUnits value) const {
        switch (value.type()) {
            case VariUnits::Framebuffer: return value.as_framebuffer();
            case VariUnits::Screen: return px_to_fb(value.as_screen());
            case VariUnits::Viewport: return vp_to_fb(value.as_viewport());
        }
        XCI_UNREACHABLE;
    }

    ScreenPixels fb_to_px(FramebufferPixels value) const {
        return value.value * screen_size().y.value / framebuffer_size().y.value;
    }

    ScreenPixels vp_to_px(ViewportUnits value) const {
        return value.value * screen_size().y.value / viewport_size().y.value;
    }

    ScreenPixels to_px(VariUnits value) const {
        switch (value.type()) {
            case VariUnits::Framebuffer: return fb_to_px(value.as_framebuffer());
            case VariUnits::Screen: return value.as_screen();
            case VariUnits::Viewport: return vp_to_px(value.as_viewport());
        }
        XCI_UNREACHABLE;
    }

    // Convert coords / size to framebuffer / screen:

    FramebufferSize px_to_fb(ScreenSize size) const {
        return {size.x.value * framebuffer_size().x.value / screen_size().x.value,
                size.y.value * framebuffer_size().y.value / screen_size().y.value};
    }

    FramebufferSize vp_to_fb(ViewportSize size) const {
        return {size.x.value * framebuffer_size().x.value / viewport_size().x.value,
                size.y.value * framebuffer_size().y.value / viewport_size().y.value};
    }

    FramebufferSize to_fb(VariSize size) const {
        assert(size.x.type() == size.y.type());
        switch (size.x.type()) {
            case VariUnits::Framebuffer: return {size.x.as_framebuffer(), size.y.as_framebuffer()};
            case VariUnits::Screen: return px_to_fb(ScreenSize{size.x.as_screen(), size.y.as_screen()});
            case VariUnits::Viewport: return vp_to_fb(ViewportSize{size.x.as_viewport(), size.y.as_viewport()});
        }
        XCI_UNREACHABLE;
    }

    ScreenSize fb_to_px(FramebufferSize size) const {
        return {size.x.value * screen_size().x.value / framebuffer_size().x.value,
                size.y.value * screen_size().y.value / framebuffer_size().y.value};
    }

    ScreenSize vp_to_px(ViewportSize size) const {
        return {size.x.value * screen_size().x.value / viewport_size().x.value,
                size.y.value * screen_size().y.value / viewport_size().y.value};
    }

    ScreenSize to_px(VariSize size) const {
        assert(size.x.type() == size.y.type());
        switch (size.x.type()) {
            case VariUnits::Framebuffer: return fb_to_px(FramebufferSize{size.x.as_framebuffer(), size.y.as_framebuffer()});
            case VariUnits::Screen: return {size.x.as_screen(), size.y.as_screen()};
            case VariUnits::Viewport: return vp_to_px(ViewportSize{size.x.as_viewport(), size.y.as_viewport()});
        }
        XCI_UNREACHABLE;
    }

    // Convert rect to framebuffer / screen:

    FramebufferRect px_to_fb(const ScreenRect& rect) const {
        auto xy = px_to_fb(rect.top_left());
        auto size = px_to_fb(rect.size());
        return {xy.x, xy.y, size.x, size.y};
    }

    /// Convert rectangle in viewport space into framebuffer space.
    FramebufferRect vp_to_fb(const ViewportRect& rect) const {
        auto xy = vp_to_fb(rect.top_left());
        auto size = vp_to_fb(rect.size());
        return {xy.x, xy.y, size.x, size.y};
    }

    FramebufferRect to_fb(const VariRect& rect) const {
        auto xy = to_fb(rect.top_left());
        auto size = to_fb(rect.size());
        return {xy.x, xy.y, size.x, size.y};
    }

    ScreenRect fb_to_px(const FramebufferRect& rect) const {
        auto xy = fb_to_px(rect.top_left());
        auto size = fb_to_px(rect.size());
        return {xy.x, xy.y, size.x, size.y};
    }

    // Convert units to viewport:

    ViewportUnits px_to_vp(ScreenPixels value) const {
        return value.value * viewport_size().y.value / screen_size().y.value;
    }

    ViewportUnits fb_to_vp(FramebufferPixels value) const {
        return value.value * viewport_size().y.value / framebuffer_size().y.value;
    }

    ViewportUnits to_vp(VariUnits value) const {
        switch (value.type()) {
            case VariUnits::Framebuffer: return fb_to_vp(value.as_framebuffer());
            case VariUnits::Screen: return px_to_vp(value.as_screen());
            case VariUnits::Viewport: return value.as_viewport();
        }
        XCI_UNREACHABLE;
    }

    ViewportSize px_to_vp(ScreenSize size) const {
        return {size.x.value * viewport_size().x.value / screen_size().x.value,
                size.y.value * viewport_size().y.value / screen_size().y.value};
    }

    ViewportSize fb_to_vp(FramebufferSize size) const {
        return {size.x.value * viewport_size().x.value / framebuffer_size().x.value,
                size.y.value * viewport_size().y.value / framebuffer_size().y.value};
    }

    ViewportRect px_to_vp(const ScreenRect& rect) const {
        auto xy = px_to_vp(rect.top_left());
        auto size = px_to_vp(rect.size());
        return {xy.x, xy.y, size.x, size.y};
    }

    ViewportRect fb_to_vp(const FramebufferRect& rect) const {
        auto xy = fb_to_vp(rect.top_left());
        auto size = fb_to_vp(rect.size());
        return {xy.x, xy.y, size.x, size.y};
    }

    // ------------------------------------------------------------------------
    // Local offset of coordinates

    template <class T>
    class PopHelper {
        friend View;
        std::vector<T>& container;
        PopHelper(std::vector<T>& c) : container(c) {}
    public:
        ~PopHelper() { container.pop_back(); }
    };

    PopHelper<FramebufferCoords> push_offset(VariCoords offset);
    FramebufferCoords offset() const;

    // ------------------------------------------------------------------------
    // Crop region (scissors test)

    PopHelper<FramebufferRect> push_crop(const FramebufferRect& region);
    bool has_crop() const { return !m_crop.empty(); }
    const FramebufferRect& get_crop() const { return m_crop.back(); }

    // ------------------------------------------------------------------------
    // Refresh

    // Demand refresh (in OnDemand refresh mode)
    // This should be called from Window::UpdateCallback to get DrawCallback
    // called afterwards. The event loop will still block waiting for another event.
    // To emulate Periodic refresh mode, call also Window::wakeup() from
    // the UpdateCallback, ie.
    //      view.refresh();
    //      view.window()->wakeup();
    // That is: redraw and generate empty event, thus skipping the next blocking
    // wait for events and returning back to UpdateCallback, which wakes up again etc.
    void refresh() { m_needs_refresh = true; }
    bool pop_refresh();

    // Wait for asynchronous draw commands to finish.
    // This needs to be called before recreating objects that are being drawn.
    void finish_draw();

    // ------------------------------------------------------------------------
    // Visual debugging

    using DebugFlags = unsigned int;
    enum class Debug: DebugFlags {
        GlyphBBox       = 1u << 0u,
        WordBBox        = 1u << 1u,
        WordBasePoint   = 1u << 2u,
        LineBBox        = 1u << 3u,
        LineBaseLine    = 1u << 4u,
        SpanBBox        = 1u << 5u,
        PageBBox        = 1u << 6u,
    };
    void set_debug_flags(DebugFlags flags) { m_debug = flags; }
    void set_debug_flag(Debug flag, bool enabled = true);
    bool has_debug_flag(Debug flag) const;

private:
    void rescale_viewport();

private:
    Window* m_window = nullptr;  // Window which created this View
    ViewportSize m_viewport_size;       // eg. {2.666, 2.0}
    ScreenSize m_screen_size;           // eg. {800, 600}
    FramebufferSize m_framebuffer_size; // eg. {1600, 1200}
    ViewOrigin m_origin = ViewOrigin::Center;
    float m_vp_scale = 100.0f;
    DebugFlags m_debug = 0;
    bool m_needs_refresh = true;  // start with dirty state to force first refresh
    std::vector<FramebufferRect> m_crop;  // Crop region stack (current crop region on back)
    std::vector<FramebufferCoords> m_offset;  // Offset stack (current offset on back)
};


} // namespace xci::graphics

template <> struct fmt::formatter<xci::graphics::FramebufferPixels> : ostream_formatter {};
template <> struct fmt::formatter<xci::graphics::ScreenPixels> : ostream_formatter {};
template <> struct fmt::formatter<xci::graphics::ViewportUnits> : ostream_formatter {};
template <typename T> struct fmt::formatter<xci::core::Vec2<T>> : ostream_formatter {};
template <typename T> struct fmt::formatter<xci::core::Rect<T>> : ostream_formatter {};

#endif // XCI_GRAPHICS_VIEW_H
