// Texture.cpp created on 2019-10-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Texture.h"
#include "Renderer.h"
#include "vulkan/VulkanError.h"
#include "vulkan/CommandBuffers.h"
#include <xci/core/log.h>
#include <xci/compat/macros.h>
#include <cassert>
#include <cstring>

namespace xci::graphics {


Texture::Texture(Renderer& renderer, ColorFormat format)
    : m_renderer(renderer), m_format(format),
      m_staging_memory(renderer), m_image_memory(renderer)
{}


bool Texture::create(const Vec2u& size)
{
    destroy();
    m_size = size;

    // staging buffer
    {
        VkBufferCreateInfo buffer_ci = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = byte_size(),
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        VK_TRY("vkCreateBuffer(staging texture)",
                vkCreateBuffer(device(), &buffer_ci, nullptr, &m_staging_buffer));
        VkMemoryRequirements mem_req;
        vkGetBufferMemoryRequirements(device(), m_staging_buffer, &mem_req);
        auto offset = m_staging_memory.reserve(mem_req);
        assert(offset == 0);
        m_staging_memory.allocate(
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_staging_memory.bind_buffer(m_staging_buffer, offset);
        m_staging_mapped = m_staging_memory.map(0, byte_size());
    }

    // image

    VkImageCreateInfo image_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = vk_format(),
            .extent = {
                    .width = size.x,
                    .height = size.y,
                    .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VK_TRY("vkCreateImage",
            vkCreateImage(device(), &image_ci, nullptr, &m_image));
    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(device(), m_image, &mem_req);

    auto offset = m_image_memory.reserve(mem_req);
    assert(offset == 0);
    m_image_memory.allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_image_memory.bind_image(m_image, offset);

    // image view

    VkImageViewCreateInfo image_view_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vk_format(),
            .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
    };
    VK_TRY("vkCreateImageView(texture)",
            vkCreateImageView(device(), &image_view_ci,
                    nullptr, &m_image_view));

    // sampler

    VkSamplerCreateInfo sampler_ci = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 1.0,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
    };

    VK_TRY("vkCreateSampler",
            vkCreateSampler(device(), &sampler_ci, nullptr,
                    &m_sampler));

    return true;
}


static constexpr size_t format_pixel_size(ColorFormat format) {
    switch (format) {
        case ColorFormat::Grey: return 1;
        case ColorFormat::BGRA: return 4;
    }
    XCI_UNREACHABLE;
}


void Texture::write(const uint8_t* pixels)
{
    assert(m_staging_mapped != nullptr);
    std::memcpy(m_staging_mapped, pixels, byte_size());
    m_pending_regions.clear();
    m_pending_regions.emplace_back(0, 0, m_size.x, m_size.y);
}


void Texture::write(const uint8_t* pixels, const Rect_u& region)
{
    assert(m_staging_mapped != nullptr);
    const auto pixel_size = format_pixel_size(m_format);
    const auto stride = region.w * pixel_size;
    for (size_t y = 0; y != region.h; ++y) {
        std::memcpy(
            (char*) m_staging_mapped + ((region.y + y) * m_size.x + region.x) * pixel_size,
            pixels, stride);
        pixels += stride;
    }
    m_pending_regions.push_back(region);
}


void Texture::clear()
{
    m_pending_clear = true;
    m_pending_regions.clear();
    std::memset(m_staging_mapped, 0, byte_size());
}


void Texture::update()
{
    if (m_pending_regions.empty())
        return;

    TRACE("write pending regions to texture");
    CommandBuffers cmd_buf(m_renderer);
    cmd_buf.create(m_renderer.vk_transient_command_pool(), 1);
    cmd_buf.begin();

    cmd_buf.transition_image_layout(m_image,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            m_image_layout,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    if (m_pending_clear) {
        m_pending_clear = false;
        VkImageSubresourceRange range {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
        };
        VkClearColorValue clear_color {};
        vkCmdClearColorImage(cmd_buf.vk(),
                m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                &clear_color, 1, &range);
    }

    for (auto& region : m_pending_regions) {
        const auto pixel_size = format_pixel_size(m_format);
        if (pixel_size % 4 != 0) {
            // offset must be a multiple of 4
            auto align = (region.y * m_size.x + region.x) % 4;
            region.x -= align;
            region.w += align;
        }
        cmd_buf.copy_buffer_to_image(m_staging_buffer,
                (region.y * m_size.x + region.x) * pixel_size,
                m_size.x, m_image, region);
    }
    m_pending_regions.clear();

    m_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    cmd_buf.transition_image_layout(m_image,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_image_layout);

    cmd_buf.end();
    cmd_buf.submit();
}


VkDeviceSize Texture::byte_size() const
{
    return size_t(m_size.x * m_size.y) * format_pixel_size(m_format);
}


VkFormat Texture::vk_format() const
{
    switch (m_format) {
        case ColorFormat::Grey: return VK_FORMAT_R8_UNORM;
        case ColorFormat::BGRA: return VK_FORMAT_B8G8R8A8_UNORM;
    }
    XCI_UNREACHABLE;
}


VkDevice Texture::device() const
{
    return m_renderer.vk_device();
}


void Texture::destroy()
{
    if (m_staging_mapped != nullptr)
        m_staging_memory.unmap();
    vkDestroySampler(device(), m_sampler, nullptr);
    vkDestroyImageView(device(), m_image_view, nullptr);
    vkDestroyImage(device(), m_image, nullptr);
    vkDestroyBuffer(device(), m_staging_buffer, nullptr);
}


} // namespace xci::graphics
