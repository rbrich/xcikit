// Swapchain.h created on 2023-10-31 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_VULKAN_SWAPCHAIN_H
#define XCI_GRAPHICS_VULKAN_SWAPCHAIN_H

#include "Image.h"
#include "Framebuffer.h"
#include "Attachments.h"

#include <vulkan/vulkan.h>

namespace xci::graphics {

class Renderer;


enum class PresentMode : uint8_t {
    Immediate,      // no vsync, possible tearing
    Mailbox,        // vsync, new request replaces old one (program is not slowed down)
    Fifo,           // vsync, requests are queued
    FifoRelaxed,    // vsync, requests are queued, late frame can be displayed immediately
};


class Swapchain {
public:
    explicit Swapchain(Renderer& renderer)
            : m_renderer(renderer), m_framebuffer(renderer) {}
    ~Swapchain() { destroy(); }

    void create();
    void destroy();

    void create_framebuffers();
    void destroy_framebuffers();
    void reset_framebuffer(VkExtent2D new_size);

    void set_present_mode(PresentMode mode);
    PresentMode present_mode() const { return m_present_mode; }

    /// Create depth buffer for the swapchain
    void set_depth_buffering(bool enable) { m_attachments.set_depth_bits(enable ? 32 : 0); }
    bool depth_buffering() const { return m_attachments.depth_bits() > 0; }

    /// Multisampling (MSAA)
    void set_sample_count(uint32_t count);
    VkSampleCountFlagBits sample_count() const { return (VkSampleCountFlagBits) m_attachments.msaa_samples(); }
    bool is_multisample() const { return m_attachments.has_msaa(); }

    void query_surface_capabilities(VkPhysicalDevice device, VkExtent2D new_size);
    bool query(VkPhysicalDevice device);

    const Attachments& attachments() const { return m_attachments; }
    Attachments& attachments() { return m_attachments; }

    const Framebuffer& framebuffer() const { return m_framebuffer; }

    VkSwapchainKHR vk() const { return m_swapchain; }
    VkSurfaceFormatKHR vk_surface_format() const { return m_surface_format; }
    VkExtent2D vk_image_extent() const { return m_extent; }
    VkFramebuffer vk_framebuffer(uint32_t index) const { return m_framebuffer[index]; }

private:
    void recreate();

    Renderer& m_renderer;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;

    Attachments m_attachments;

    VkImage m_images[Framebuffer::max_image_count] {};
    Framebuffer m_framebuffer;

    // create info
    VkSurfaceFormatKHR m_surface_format {};
    VkExtent2D m_extent {};
    uint32_t m_image_count = 0;  // <= max_image_count
    PresentMode m_present_mode = PresentMode::Fifo;
};


} // namespace xci::graphics

#endif // include guard
