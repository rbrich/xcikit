// View.h created on 2018-03-04, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef XCI_GRAPHICS_VIEW_H
#define XCI_GRAPHICS_VIEW_H

#include <xci/core/geometry.h>

#include <memory>
#include <vector>
#include <array>

namespace xci::graphics {


enum class Unit {
    ScreenPixel,        // virtual screen pixels
    FramebufferPixel,   // actual GPU pixels
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
    Units operator +(Units rhs) const { return value + rhs.value; }
    Units operator -(Units rhs) const { return value - rhs.value; }
    Units operator *(Units rhs) const { return value * rhs.value; }
    Units operator /(Units rhs) const { return value / rhs.value; }
    Units operator +=(Units rhs) { value += rhs.value; return value; }
    Units operator -=(Units rhs) { value -= rhs.value; return value; }
    friend Units operator *(T lhs, Units rhs) { return lhs * rhs.value; }
    friend Units operator +(T lhs, Units rhs) { return lhs + rhs.value; }
    friend Units operator -(T lhs, Units rhs) { return lhs - rhs.value; }
    friend std::ostream& operator <<(std::ostream& s, Units rhs) { return s << rhs.value; }

    constexpr Unit unit() const { return u; }

    template <typename U>
    U as() const { return static_cast<U>(value); }

    // Value of Units can be explicitly casted to T
    using numeric_type = T;
    constexpr explicit operator T() const { return value; }

    T value = 0;
};


using ScreenPixels = Units<float, Unit::ScreenPixel>;
using ScreenCoords = core::Vec2<ScreenPixels>;
using ScreenSize = core::Vec2<ScreenPixels>;
using ScreenRect = core::Rect<ScreenPixels>;

using FramebufferPixels = Units<float, Unit::FramebufferPixel>;
using FramebufferCoords = core::Vec2<FramebufferPixels>;
using FramebufferSize = core::Vec2<FramebufferPixels>;
using FramebufferRect = core::Rect<FramebufferPixels>;

using ViewportUnits = Units<float, Unit::ViewportUnit>;
using ViewportCoords = core::Vec2<ViewportUnits>;
using ViewportSize = core::Vec2<ViewportUnits>;
using ViewportRect = core::Rect<ViewportUnits>;

namespace unit_literals {
constexpr ScreenPixels operator ""_sc (long double value) { return ScreenPixels(value); }
constexpr ScreenPixels operator ""_sc (unsigned long long value) { return ScreenPixels(value); }
constexpr FramebufferPixels operator ""_fb (long double value) { return FramebufferPixels(value); }
constexpr FramebufferPixels operator ""_fb (unsigned long long value) { return FramebufferPixels(value); }
constexpr ViewportUnits operator ""_vp (long double value) { return ViewportUnits(value); }
constexpr ViewportUnits operator ""_vp (unsigned long long value) { return ViewportUnits(value); }
} // namespace unit_literals


enum class ViewOrigin {
    Center,
    TopLeft,
};

enum class ViewScale {
    ScalingWithAspectCorrection,    // 2.0 x 2.0 + horiz/vert extension
    FixedScreenPixels,              // same as screen_size()
};


class Window;

class View {
public:
    View() = default;
    explicit View(Window* m_window) : m_window(m_window) {}

    // Window might be nullptr if the View was created directly
    Window* window() const { return m_window; }

    // Compute projection matrix based on viewport size and offset
    std::array<float, 16> projection_matrix(bool inverted_y = true) const;

    // ------------------------------------------------------------------------
    // Sizes, coordinates

    // Size of the view in screen pixels. This size might be different
    // from framebuffer size - in that case, call also `set_framebuffer_size`.
    bool set_screen_size(ScreenSize size);
    ScreenSize screen_size() const { return m_screen_size; }

    // Size of the view in framebuffer pixels.
    // This is used for pixel-perfect font rendering.
    // By default (or when set to {0, 0}, the framebuffer size will be set
    // to same value as view size in screen pixels.
    bool set_framebuffer_size(FramebufferSize size);
    FramebufferSize framebuffer_size() const { return m_framebuffer_size; }

    // Size of the view in "viewport" units. These units are used
    // for placing objects in the view. Default view size is at least 2 units
    // in either direction, with center at {0, 0} and aspect correction.
    // X goes right, Y goes down. Total size in one of the dimensions
    // will always equal 2.0.
    // Eg: {2.666, 2.0} for 800x600 (4/3 aspect ratio)
    void set_viewport_mode(ViewOrigin origin, ViewScale scale);
    ViewportSize viewport_size() const { return m_viewport_size; }
    ViewportCoords viewport_center() const;

    // Conversion units to framebuffer:

    FramebufferPixels size_to_framebuffer(ScreenPixels value) const {
        return value.value * framebuffer_size().y.value / screen_size().y.value;
    }

    FramebufferPixels size_to_framebuffer(ViewportUnits value) const {
        return value.value * framebuffer_size().y.value / viewport_size().y.value;
    }

    FramebufferSize size_to_framebuffer(ViewportSize size) const {
        core::Vec2f s = {size.x.value * framebuffer_size().x.value,
                         size.y.value * framebuffer_size().y.value};
        return {s.x / viewport_size().x.value,
                s.y / viewport_size().y.value};
    }

    /// Convert coords in viewport units into framebuffer coords.
    /// Framebuffer has zero coords in bottom-left corner, inverted Y axis.
    FramebufferCoords coords_to_framebuffer(const ViewportCoords& coords) const {
        auto c = size_to_framebuffer(coords + 0.5f * viewport_size());
        return {c.x, framebuffer_size().y - c.y};
    }

    /// Convert rectangle in viewport space into framebuffer space.
    FramebufferRect rect_to_framebuffer(const ViewportRect& rect) const {
        auto xy = coords_to_framebuffer(rect.top_left());
        auto size = size_to_framebuffer(rect.size());
        return {xy.x, xy.y - size.y, size.x, size.y};
    }

    // Convert units to viewport:

    ViewportUnits size_to_viewport(ScreenPixels value) const {
        return value.value * viewport_size().y.value / screen_size().y.value;
    }

    ViewportSize size_to_viewport(ScreenSize size) const {
        core::Vec2f s = {size.x.value * viewport_size().x.value,
                         size.y.value * viewport_size().y.value};
        return {s.x / screen_size().x.value,
                s.y / screen_size().y.value};
    }

    ViewportUnits size_to_viewport(FramebufferPixels value) const {
        return value.value * viewport_size().y.value / framebuffer_size().y.value;
    }

    ViewportSize size_to_viewport(FramebufferSize size) const {
        core::Vec2f s = {size.x.value * viewport_size().x.value,
                         size.y.value * viewport_size().y.value};
        return {s.x / framebuffer_size().x.value,
                s.y / framebuffer_size().y.value};
    }

    ViewportCoords coords_to_viewport(const ScreenCoords& coords) const {
        return size_to_viewport(coords) - 0.5f * viewport_size();
    }

    // ------------------------------------------------------------------------
    // Local offset of coordinates

    void push_offset(const ViewportCoords& offset);
    void pop_offset() { m_offset.pop_back(); }
    const ViewportCoords& offset() const;

    // ------------------------------------------------------------------------
    // Crop region (scissors test)

    void push_crop(const ViewportRect& region);
    void pop_crop() { m_crop.pop_back(); }
    bool has_crop() const { return !m_crop.empty(); }
    const ViewportRect& get_crop() const { return m_crop.back(); }

    // ------------------------------------------------------------------------
    // Refresh

    // Demand refresh (in OnDemand refresh mode)
    // This should be called from Window::UpdateCallback to get DrawCallback
    // called afterwards. The event loop will still block waiting for another event.
    // To emulate Periodic refresh mode, call also Window::wakeup() from
    // the UpdateCallback, ie.
    //      view.refresh();
    //      view.window()->wakeup();
    // That is: redraw and generate empty event, thus skiping the next blocking
    // wait for events and returning back to UpdateCallback, which wakes up again etc.
    void refresh() { m_needs_refresh = true; }
    bool pop_refresh();

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
    ViewScale m_scale = ViewScale::ScalingWithAspectCorrection;
    DebugFlags m_debug = 0;
    bool m_needs_refresh = true;  // start with dirty state to force first refresh
    std::vector<ViewportRect> m_crop;  // Crop region stack (current crop region on back)
    std::vector<ViewportCoords> m_offset;  // Offset stack (current offset on back)
};


} // namespace xci::graphics

#endif // XCI_GRAPHICS_VIEW_H
