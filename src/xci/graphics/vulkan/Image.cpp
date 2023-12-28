// Image.cpp created on 2023-12-26 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Image.h"
#include "VulkanError.h"
#include <xci/graphics/Renderer.h>
#include <cassert>

namespace xci::graphics {


ImageCreateInfo::ImageCreateInfo(const Vec2u& size, VkFormat format,
                                 VkImageUsageFlags usage)
    : m_image_ci{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = {
                    .width = size.x,
                    .height = size.y,
                    .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      }
{}


void Image::create(const ImageCreateInfo& image_ci)
{
    auto* device = m_renderer.vk_device();
    VK_TRY("vkCreateImage",
           vkCreateImage(device, &image_ci.vk(), nullptr, &m_image));

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(device, m_image, &mem_req);

    auto offset = m_image_memory.reserve(mem_req);
    assert(offset == 0);
    m_image_memory.allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_image_memory.bind_image(m_image, offset);
}


void Image::destroy()
{
    if (m_image) {
        vkDestroyImage(m_renderer.vk_device(), m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        m_image_memory.free();
    }
}


void ImageView::create(VkDevice device, VkImage image, VkFormat format,
                       VkImageAspectFlags aspect_mask)
{
    const VkImageViewCreateInfo image_view_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange = {
                    .aspectMask = aspect_mask,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
    };
    VK_TRY("vkCreateImageView",
            vkCreateImageView(device, &image_view_ci, nullptr, &m_image_view));
}


void ImageView::destroy(VkDevice device)
{
    vkDestroyImageView(device, m_image_view, nullptr);
    m_image_view = VK_NULL_HANDLE;
}


} // namespace xci::graphics
