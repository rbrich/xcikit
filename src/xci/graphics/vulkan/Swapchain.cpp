// Swapchain.cpp created on 2023-10-31 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Swapchain.h"
#include "VulkanError.h"
#include <xci/graphics/Renderer.h>
#include <xci/core/log.h>
#include <cassert>

namespace xci::graphics {

using namespace xci::core;


static_assert(int(PresentMode::Immediate) == int(VK_PRESENT_MODE_IMMEDIATE_KHR));
static_assert(int(PresentMode::Mailbox) == int(VK_PRESENT_MODE_MAILBOX_KHR));
static_assert(int(PresentMode::Fifo) == int(VK_PRESENT_MODE_FIFO_KHR));
static_assert(int(PresentMode::FifoRelaxed) == int(VK_PRESENT_MODE_FIFO_RELAXED_KHR));


static VkPresentModeKHR present_mode_to_vk(PresentMode mode)
{
    switch (mode) {
        case PresentMode::Immediate:    return VK_PRESENT_MODE_IMMEDIATE_KHR;
        case PresentMode::Mailbox:      return VK_PRESENT_MODE_MAILBOX_KHR;
        case PresentMode::Fifo:         return VK_PRESENT_MODE_FIFO_KHR;
        case PresentMode::FifoRelaxed:  return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    }
    XCI_UNREACHABLE;
}


static const char* present_mode_to_str(PresentMode mode)
{
    switch (mode) {
        case PresentMode::Immediate:    return "Immediate";
        case PresentMode::Mailbox:      return "Mailbox";
        case PresentMode::Fifo:         return "Fifo";
        case PresentMode::FifoRelaxed:  return "FifoRelaxed";
    }
    XCI_UNREACHABLE;
}


void Swapchain::create()
{
    if (m_attachments.color_attachment_count() == 0) {
        m_attachments.add_color_attachment(vk_surface_format().format,
                                           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    } else {
        m_attachments.set_color_attachment(0, vk_surface_format().format,
                                           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    const auto device = m_renderer.vk_device();
    const VkSwapchainCreateInfoKHR swapchain_create_info {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_renderer.vk_surface(),
            .minImageCount = m_image_count,
            .imageFormat = m_surface_format.format,
            .imageColorSpace = m_surface_format.colorSpace,
            .imageExtent = m_extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = present_mode_to_vk(m_present_mode),
            .clipped = VK_TRUE,
            .oldSwapchain = m_swapchain,
    };

    VkSwapchainKHR new_swapchain = VK_NULL_HANDLE;
    VK_TRY("vkCreateSwapchainKHR",
            vkCreateSwapchainKHR(device, &swapchain_create_info,
                    nullptr, &new_swapchain));

    destroy();
    m_swapchain = new_swapchain;

    TRACE("Vulkan: swapchain image count: {}", m_image_count);
    vkGetSwapchainImagesKHR(device, m_swapchain, &m_image_count, nullptr);

    if (m_image_count > Framebuffer::max_image_count)
        VK_THROW("vulkan: too many swapchain images");

    vkGetSwapchainImagesKHR(device, m_swapchain, &m_image_count, m_images);
}


void Swapchain::destroy()
{
    const auto device = m_renderer.vk_device();
    if (device != VK_NULL_HANDLE && m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, m_swapchain, nullptr);
        m_swapchain = nullptr;
    }
}


void Swapchain::create_framebuffers()
{
    m_framebuffer.create(m_attachments, m_extent, m_image_count, m_images);
}


void Swapchain::destroy_framebuffers()
{
    m_framebuffer.destroy();
}


void Swapchain::reset_framebuffer(VkExtent2D new_size)
{
    vkDeviceWaitIdle(m_renderer.vk_device());

    query_surface_capabilities(m_renderer.vk_physical_device(), new_size);
    if (!query(m_renderer.vk_physical_device()))
        VK_THROW("vulkan: physical device no longer usable");

    recreate();

    TRACE("framebuffer resized to {}x{}", m_extent.width, m_extent.height);
}


void Swapchain::set_present_mode(PresentMode mode)
{
    m_present_mode = mode;

    // not yet initialized
    if (!m_renderer.vk_surface())
        return;

    vkDeviceWaitIdle(m_renderer.vk_device());

    if (!query(m_renderer.vk_physical_device()))
        VK_THROW("vulkan: physical device no longer usable");

    recreate();
}


void Swapchain::set_sample_count(uint32_t count)
{
    // Let's assume the VkSampleCountFlagBits values are same as actual sample count
    m_attachments.set_msaa_samples(std::min(std::max(count, 1u),
                                   uint32_t(VK_SAMPLE_COUNT_64_BIT)));
}


void Swapchain::query_surface_capabilities(VkPhysicalDevice device, VkExtent2D new_size)
{
    VkSurfaceCapabilitiesKHR capabilities;
    VK_TRY("vkGetPhysicalDeviceSurfaceCapabilitiesKHR",
           vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                   device, m_renderer.vk_surface(), &capabilities));

    if (capabilities.currentExtent.width != UINT32_MAX)
        m_extent = capabilities.currentExtent;
    else if (new_size.width != UINT32_MAX)
        m_extent = new_size;

    m_extent.width = std::clamp(m_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    m_extent.height = std::clamp(m_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    // evaluate min image count
    m_image_count = std::max(3u, capabilities.minImageCount);
    if (capabilities.maxImageCount > 0 && m_image_count > capabilities.maxImageCount)
        m_image_count = capabilities.maxImageCount;
}


bool Swapchain::query(VkPhysicalDevice device)
{
    const auto surface = m_renderer.vk_surface();

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, formats.data());

    bool fmt_found = false;
    for (const auto& fmt : formats) {
        if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            m_surface_format = fmt;
            fmt_found = true;
            break;
        }
    }
    if (!fmt_found && format_count > 0) {
        log::error("vulkan: surface format not supported: VK_FORMAT_B8G8R8A8_SRGB / VK_COLOR_SPACE_SRGB_NONLINEAR_KHR");
        return false;
    }

    uint32_t mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &mode_count, nullptr);
    std::vector<VkPresentModeKHR> modes(mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &mode_count, modes.data());

    if (format_count == 0 || mode_count == 0)
        return false;

    bool mode_found = std::find(modes.begin(), modes.end(), present_mode_to_vk(m_present_mode)) != modes.end();
    if (!mode_found) {
        const auto selected_mode = static_cast<PresentMode>(modes[0]);
        log::warning("vulkan: requested present mode not supported: {}, falling back to {}",
                   present_mode_to_str(m_present_mode), present_mode_to_str(selected_mode));
        m_present_mode = selected_mode;
    }

    return true;
}


void Swapchain::recreate()
{
    destroy_framebuffers();
    create();
    create_framebuffers();
}


} // namespace xci::graphics
