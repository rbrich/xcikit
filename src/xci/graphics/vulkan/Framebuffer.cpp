// Framebuffer.cpp created on 2024-01-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Framebuffer.h"
#include "VulkanError.h"
#include <xci/graphics/Renderer.h>

namespace xci::graphics {


VkDeviceSize Framebuffer::create_image(const ImageCreateInfo& image_ci, VkImage& image)
{
    VkDevice device = m_renderer.vk_device();
    VK_TRY("vkCreateImage",
           vkCreateImage(device, &image_ci.vk(), nullptr, &image));

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(device, image, &mem_req);
    return m_image_memory.reserve(mem_req);  // offset
}


void Framebuffer::create(const Attachments& attachments, VkExtent2D size, uint32_t image_count,
                         VkImage* swapchain_images)
{
    if (m_framebuffers[0])
        destroy();

    struct BindImage {
        VkImage image;
        VkDeviceSize offset;
    };
    std::vector<BindImage> deferred_bind;
    struct View {
        VkImage image;
        VkFormat format;
        VkImageAspectFlags aspect;
    };
    std::vector<View> deferred_views;
    const auto create_image_defer = [this, &deferred_bind](const ImageCreateInfo& image_ci, VkImage& image) {
        auto offset = create_image(image_ci, image);
        deferred_bind.push_back({image, offset});
    };

    m_image_count = image_count;
    const auto device = m_renderer.vk_device();

    // Prepare color buffers
    for (const auto& color : attachments.color_attachments()) {
        if (swapchain_images) {
            m_borrowed_count = m_image_count;
            for (unsigned i = 0; i < m_image_count; i++) {
                auto* image = m_images.emplace_back(swapchain_images[i]);
                deferred_views.push_back({image, color.format, VK_IMAGE_ASPECT_COLOR_BIT});
            }
            swapchain_images = nullptr;
            continue;
        }
        for (unsigned i = 0; i < m_image_count; i++) {
            ImageCreateInfo image_ci{{size.width, size.height}, color.format,
                                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                     color.usage};
            auto& image = m_images.emplace_back();
            create_image_defer(image_ci, image);
            deferred_views.push_back({image, color.format, VK_IMAGE_ASPECT_COLOR_BIT});
        }
    }

    // Prepare depth buffer
    if (attachments.has_depth()) {
        VkFormat format = attachments.depth_stencil_format();
        ImageCreateInfo image_ci{{size.width, size.height}, format,
                                 VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT};
        image_ci.set_samples(attachments.msaa_samples_flag());
        auto& image = m_images.emplace_back();
        create_image_defer(image_ci, image);  // VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT
        deferred_views.push_back({image, format, VK_IMAGE_ASPECT_DEPTH_BIT});
    }

    // Prepare MSAA color buffers
    if (attachments.has_msaa()) {
        for (const auto& color : attachments.color_attachments()) {
            ImageCreateInfo image_ci{{size.width, size.height}, color.format,
                                     VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};
            image_ci.set_samples(attachments.msaa_samples_flag());
            auto& image = m_images.emplace_back();
            create_image_defer(image_ci, image); // VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT
            deferred_views.push_back({image, color.format, VK_IMAGE_ASPECT_COLOR_BIT});
        }
    }

    // Allocate memory, bind images
    m_image_memory.allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    for (const auto& deferred : deferred_bind) {
        m_image_memory.bind_image(deferred.image, deferred.offset);
    }

    // Create image views
    m_image_views.reserve(deferred_views.size());
    for (const auto& deferred : deferred_views) {
        auto& view = m_image_views.emplace_back();
        view.create(device, deferred.image, deferred.format, deferred.aspect);
    }

    // Create framebuffers
    for (size_t i = 0; i < m_image_count; i++) {
        std::vector<VkImageView> attachment_views;
        uint32_t base = 0;
        for (uint32_t a = 0; a != attachments.color_attachment_count(); ++a) {
            attachment_views.push_back(m_image_views[base + i].vk());
            base += m_image_count;
        }
        if (attachments.has_depth_stencil()) {
            attachment_views.push_back(m_image_views[base].vk());
            base += 1;
        }
        if (attachments.has_msaa()) {
            for (uint32_t r = 0; r != attachments.color_attachment_count(); ++r) {
                // swap color buffer with MSAA color buffer
                attachment_views.push_back(attachment_views[r]);  // original buffer used as resolve
                attachment_views[r] = m_image_views[base + r].vk();    // MSAA color buffer with multisample
            }
        }

        const VkFramebufferCreateInfo framebuffer_ci = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = attachments.render_pass(),
                .attachmentCount = (uint32_t) attachment_views.size(),
                .pAttachments = attachment_views.data(),
                .width = size.width,
                .height = size.height,
                .layers = 1,
        };

        VK_TRY("vkCreateFramebuffer",
                vkCreateFramebuffer(m_renderer.vk_device(), &framebuffer_ci,
                        nullptr, &m_framebuffers[i]));
    }
}


void Framebuffer::destroy()
{
    VkDevice device = m_renderer.vk_device();
    if (device == VK_NULL_HANDLE)
        return;
    for (uint32_t i = 0; i != m_image_count; ++i) {
        vkDestroyFramebuffer(device, m_framebuffers[i], nullptr);
        m_framebuffers[i] = VK_NULL_HANDLE;
    }
    m_image_count = 0;
    for (auto& image_view : m_image_views) {
        image_view.destroy(device);
    }
    m_image_views.clear();
    for (unsigned i = 0; i != m_borrowed_count; ++i) {
        m_images[i] = VK_NULL_HANDLE;
    }
    m_borrowed_count = 0;
    for (auto* image : m_images) {
        vkDestroyImage(device, image, nullptr);
    }
    m_images.clear();
    m_image_memory.free();
}


} // namespace xci::graphics
