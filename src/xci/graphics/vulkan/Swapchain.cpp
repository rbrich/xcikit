// Swapchain.cpp created on 2023-10-31 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Swapchain.h"
#include "VulkanError.h"
#include <xci/graphics/Renderer.h>
#include <xci/core/log.h>

#include <range/v3/view/take.hpp>

namespace xci::graphics {

using namespace xci::core;
using ranges::cpp20::views::take;


static_assert(int(PresentMode::Immediate) == int(VK_PRESENT_MODE_IMMEDIATE_KHR));
static_assert(int(PresentMode::Mailbox) == int(VK_PRESENT_MODE_MAILBOX_KHR));
static_assert(int(PresentMode::Fifo) == int(VK_PRESENT_MODE_FIFO_KHR));
static_assert(int(PresentMode::FifoRelaxed) == int(VK_PRESENT_MODE_FIFO_RELAXED_KHR));


VkPresentModeKHR present_mode_to_vk(PresentMode mode)
{
    switch (mode) {
        case PresentMode::Immediate:    return VK_PRESENT_MODE_IMMEDIATE_KHR;
        case PresentMode::Mailbox:      return VK_PRESENT_MODE_MAILBOX_KHR;
        case PresentMode::Fifo:         return VK_PRESENT_MODE_FIFO_KHR;
        case PresentMode::FifoRelaxed:  return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    }
    XCI_UNREACHABLE;
}


const char* present_mode_to_str(PresentMode mode)
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
            .oldSwapchain = VK_NULL_HANDLE,
    };

    VK_TRY("vkCreateSwapchainKHR",
            vkCreateSwapchainKHR(device, &swapchain_create_info,
                    nullptr, &m_swapchain));

    TRACE("Vulkan: swapchain image count: {}", m_image_count);
    vkGetSwapchainImagesKHR(device, m_swapchain,
            &m_image_count, nullptr);

    if (m_image_count > max_image_count)
        VK_THROW("vulkan: too many swapchain images");

    vkGetSwapchainImagesKHR(device, m_swapchain, &m_image_count, m_images);
    assert(m_image_count <= max_image_count);

    VkImageViewCreateInfo image_view_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_surface_format.format,
            .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
    };
    for (size_t i = 0; i < m_image_count; i++) {
        image_view_ci.image = m_images[i];
        VK_TRY("vkCreateImageView",
                vkCreateImageView(device, &image_view_ci,
                        nullptr, &m_image_views[i]));
    }
}


void Swapchain::destroy()
{
    const auto device = m_renderer.vk_device();
    if (device != VK_NULL_HANDLE && m_swapchain != VK_NULL_HANDLE) {
        for (auto image_view : m_image_views | take(m_image_count)) {
            vkDestroyImageView(device, image_view, nullptr);
        }
        vkDestroySwapchainKHR(device, m_swapchain, nullptr);
        m_swapchain = nullptr;
    }
}


void Swapchain::create_framebuffers()
{
    for (size_t i = 0; i < m_image_count; i++) {
        VkImageView attachments[] = { m_image_views[i] };

        const VkFramebufferCreateInfo framebuffer_ci = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = m_renderer.vk_render_pass(),
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = m_extent.width,
                .height = m_extent.height,
                .layers = 1,
        };

        VK_TRY("vkCreateFramebuffer",
                vkCreateFramebuffer(m_renderer.vk_device(), &framebuffer_ci,
                        nullptr, &m_framebuffers[i]));
    }
}


void Swapchain::destroy_framebuffers()
{
    for (auto framebuffer : m_framebuffers | take(m_image_count)) {
        vkDestroyFramebuffer(m_renderer.vk_device(), framebuffer, nullptr);
    }
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
    m_image_count = capabilities.minImageCount + 1;
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
        if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            m_surface_format = fmt;
            fmt_found = true;
            break;
        }
    }
    if (!fmt_found && format_count > 0) {
        log::error("vulkan: surface format not supported: VK_FORMAT_B8G8R8A8_UNORM / VK_COLOR_SPACE_SRGB_NONLINEAR_KHR");
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
    destroy();
    create();
    create_framebuffers();
}


} // namespace xci::graphics