// Framebuffer.h created on 2024-01-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_FRAMEBUFFER_H
#define XCI_GRAPHICS_FRAMEBUFFER_H

#include "DeviceMemory.h"
#include "Image.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace xci::graphics {

class Attachments;


class Framebuffer {
public:
    static constexpr uint32_t max_image_count = 3;

    explicit Framebuffer(Renderer& renderer)
            : m_renderer(renderer), m_image_memory(renderer) {}
    ~Framebuffer() { destroy(); }

    void create(const Attachments& attachments, VkExtent2D size, uint32_t image_count,
                VkImage* swapchain_images = nullptr);
    void destroy();

    VkImage color_image(uint32_t buffer, uint32_t image_index) const { return m_images[buffer * m_image_count + image_index]; }
    VkImageView color_image_view(uint32_t buffer, uint32_t image_index) const { return m_image_views[buffer * m_image_count + image_index].vk(); }

    VkFramebuffer vk_framebuffer(uint32_t index) const { return m_framebuffers[index]; }
    VkFramebuffer operator[](uint32_t index) const { return m_framebuffers[index]; }

private:
    VkDeviceSize create_image(const ImageCreateInfo& image_ci, VkImage& image);

    Renderer& m_renderer;
    VkFramebuffer m_framebuffers[max_image_count] {};
    DeviceMemory m_image_memory;

    // Images in following order and counts:
    // - N * C  color buffers
    // - 1      depth buffer
    // - C      MSAA color buffers
    // Where N = image count, C = color attachment count
    std::vector<VkImage> m_images;
    std::vector<ImageView> m_image_views;

    uint32_t m_image_count = 0;  // <= max_image_count
    uint32_t m_borrowed_count = 0; // number of borrowed swapchain images (at beginning of m_images)
};


} // namespace xci::graphics

#endif  // XCI_GRAPHICS_FRAMEBUFFER_H
