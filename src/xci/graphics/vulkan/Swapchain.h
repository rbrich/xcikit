// Swapchain.h created on 2023-10-31 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_VULKAN_SWAPCHAIN_H
#define XCI_GRAPHICS_VULKAN_SWAPCHAIN_H

#include "Image.h"
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
    explicit Swapchain(Renderer& renderer) : m_renderer(renderer), m_depth_image(renderer) {}
    ~Swapchain() { destroy(); }

    void create();
    void destroy();

    void create_framebuffers();
    void destroy_framebuffers();
    void reset_framebuffer(VkExtent2D new_size);

    void set_present_mode(PresentMode mode);
    PresentMode present_mode() const { return m_present_mode; }

    /// Create depth buffer for the swapchain
    void set_depth_buffering(bool enable) { m_depth_buffering = enable; }
    bool depth_buffering() const { return m_depth_buffering; }

    void query_surface_capabilities(VkPhysicalDevice device, VkExtent2D new_size);
    bool query(VkPhysicalDevice device);

    VkSwapchainKHR vk() const { return m_swapchain; }
    VkSurfaceFormatKHR vk_surface_format() const { return m_surface_format; }
    VkExtent2D vk_image_extent() const { return m_extent; }
    VkFramebuffer vk_framebuffer(uint32_t index) const { return m_framebuffers[index]; }

private:
    void recreate();

    Renderer& m_renderer;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;

    static constexpr uint32_t max_image_count = 8;
    VkImage m_images[max_image_count] {};
    ImageView m_image_views[max_image_count];
    VkFramebuffer m_framebuffers[max_image_count] {};

    // depth buffer
    Image m_depth_image;
    ImageView m_depth_image_view;

    // create info
    VkSurfaceFormatKHR m_surface_format {};
    VkExtent2D m_extent {};
    uint32_t m_image_count = 0;  // <= max_image_count
    PresentMode m_present_mode = PresentMode::Fifo;
    bool m_depth_buffering = false;  // create depth buffer?
};


} // namespace xci::graphics

#endif // include guard
