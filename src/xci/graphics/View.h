// View.h created on 2018-03-04, part of XCI toolkit
// Copyright 2018 Radek Brich
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

#include <xci/util/geometry.h>

#include <memory>
#include <vector>

namespace xci {
namespace graphics {

using xci::util::Vec2f;
using xci::util::Vec2u;
using xci::util::Vec2i;
using xci::util::Rect_f;
using xci::util::Rect_i;


class View {
public:
    virtual ~View() = default;

    // Size of the view in screen pixels. This size might be different
    // from framebuffer size - in that case, call also `set_framebuffer_size`.
    void set_screen_size(Vec2u size);
    Vec2u screen_size() const { return m_screen_size; }

    // Size of the view in framebuffer pixels.
    // This is used for pixel-perfect font rendering.
    // By default (or when set to {0, 0}, the framebuffer size will be set
    // to same value as view size in screen pixels.
    void set_framebuffer_size(Vec2u size);
    Vec2u framebuffer_size() const { return m_framebuffer_size; }

    // Size of the view in scalable units. These units are used
    // for placing objects in the view. The view size is at least 2 units
    // in either direction, with center at {0, 0} and aspect correction.
    // X goes right, Y goes down. Total size in one of the dimensions
    // will always equal 2.0.
    // Eg: {2.666, 2.0} for 800x600 (4/3 aspect ratio)
    Vec2f scalable_size() const { return m_scalable_size; }

    // Ratio of scalable units to screen pixels, ie.
    // size of 1x1 screen pixel in scalable units.
    Vec2f screen_ratio() const {
        auto s = scalable_size();
        auto px = screen_size();
        return {s.x / px.x, s.y / px.y};
    }

    // Ratio of scalable units to framebuffer pixels, ie.
    // size of 1x1 framebuffer pixel in scalable units.
    Vec2f framebuffer_ratio() const {
        auto s = scalable_size();
        auto fb = framebuffer_size();
        return {s.x / fb.x, s.y / fb.y};
    }

    Vec2f screen_to_scalable(const Vec2f& screen_coords) const {
        return screen_coords * screen_ratio() - 0.5f * scalable_size();
    }

    /// Convert coords in scalable units into framebuffer coords.
    /// Framebuffer has zero coords in bottom-left corner, inverted Y axis.
    Vec2i scalable_to_framebuffer(const Vec2f& scalable_coords) const {
        Vec2f c = (scalable_coords + 0.5f * scalable_size()) / framebuffer_ratio();
        return {int(c.x), int(framebuffer_size().y - c.y)};
    }

    /// Convert rectangle in scalable space into framebuffer space.
    Rect_i scalable_to_framebuffer(const Rect_f& rect) const {
        Vec2i xy = scalable_to_framebuffer(rect.top_left());
        Vec2f size = rect.size() / framebuffer_ratio();
        return {xy.x, xy.y - int(size.y), int(size.x), int(size.y)};
    }

    // ------------------------------------------------------------------------
    // Crop region (scissors test)

    void push_crop(const Rect_f& region);
    void pop_crop() { m_crop.pop_back(); }
    bool has_crop() const { return !m_crop.empty(); }
    const Rect_f& get_crop() const { return m_crop.back(); }

    // ------------------------------------------------------------------------
    // Refresh

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
        SpanBBox        = 1u << 4u,
        PageBBox        = 1u << 5u,
    };
    void set_debug_flag(Debug flag, bool enabled = true);
    void set_debug_flags(DebugFlags flags) { m_debug = flags; }
    bool has_debug_flag(Debug flag) const;

private:
    Vec2f m_scalable_size;      // eg. {2.666, 2.0}
    Vec2u m_screen_size;        // eg. {800, 600}
    Vec2u m_framebuffer_size;   // eg. {1600, 1200}
    DebugFlags m_debug = 0;
    bool m_needs_refresh = false;
    std::vector<Rect_f> m_crop;  // Crop region stack (current crop region on top)
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_VIEW_H
