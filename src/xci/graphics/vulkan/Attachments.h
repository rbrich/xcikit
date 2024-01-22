// Attachments.h created on 2024-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_ATTACHMENTS_H
#define XCI_GRAPHICS_ATTACHMENTS_H

#include <vulkan/vulkan.h>
#include <vector>
#include <cassert>

namespace xci::graphics {


class Attachments {
public:
    /// Multisampling (MSAA)
    void set_msaa_samples(uint8_t count) { m_msaa_samples = count; }
    uint8_t msaa_samples() const { return m_msaa_samples; }
    VkSampleCountFlagBits msaa_samples_flag() const { return (VkSampleCountFlagBits) m_msaa_samples; }
    bool has_msaa() const { return m_msaa_samples > 1; }

    /// Depth buffer
    void set_depth_bits(uint8_t bits) { m_depth_bits = bits; }
    uint8_t depth_bits() const { return m_depth_bits; }
    bool has_depth() const { return m_depth_bits > 0; }

    /// Stencil buffer
    void set_stencil_bits(uint8_t bits) { m_stencil_bits = bits; }
    uint8_t stencil_bits() const { return m_stencil_bits; }
    bool has_stencil() const { return m_stencil_bits > 0; }

    /// Depth and/or stencil attachment is enabled
    bool has_depth_stencil() const { return has_depth() || has_stencil(); }
    VkFormat depth_stencil_format() const;

    struct ColorAttachment {
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkImageLayout final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    };
    const std::vector<ColorAttachment>& color_attachments() const { return m_color_attachments; }

    /// Add a color attachment, starting with location=0.
    /// None exists initially.
    /// \returns Attachment reference number (location)
    uint32_t add_color_attachment(VkFormat format, VkImageLayout final_layout) {
        m_color_attachments.emplace_back(ColorAttachment{format, final_layout});
        return m_color_attachments.size() - 1;
    }
    void set_color_attachment(uint32_t location, VkFormat format, VkImageLayout final_layout) {
        assert(m_color_attachments.size() > location);
        m_color_attachments[location] = ColorAttachment{format, final_layout};
    }
    void clear_color_attachments() { m_color_attachments.clear(); }
    size_t color_attachment_count() const { return m_color_attachments.size(); }

    void create_renderpass(VkDevice device);
    void destroy_renderpass(VkDevice device);
    VkRenderPass render_pass() const { return m_render_pass; }

private:
    VkSampleCountFlagBits sample_count_flag() const { return (VkSampleCountFlagBits) m_msaa_samples; }

    VkRenderPass m_render_pass = nullptr;
    std::vector<ColorAttachment> m_color_attachments;

    uint8_t m_depth_bits = 0;  // 0 (disabled) | 16 | 24 | 32
    uint8_t m_stencil_bits = 0;  // 0 (disabled) | 8
    uint8_t m_msaa_samples = 1;  // 1 (no multisampling) 2 | 4 | 8 | 16 ...
};


} // namespace xci::graphics

#endif  // XCI_GRAPHICS_ATTACHMENTS_H
