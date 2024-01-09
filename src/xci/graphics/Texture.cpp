// Texture.cpp created on 2019-10-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Texture.h"
#include "Renderer.h"
#include "vulkan/VulkanError.h"
#include "vulkan/CommandBuffers.h"
#include <xci/compat/macros.h>
#include <bit>
#include <cassert>
#include <cstring>

namespace xci::graphics {


uint32_t mip_levels_for_size(Vec2u size)
{
    return std::bit_width(size.x | size.y);
}


Texture::Texture(Renderer& renderer)
    : m_renderer(renderer),
      m_image(renderer),
      m_staging_memory(renderer)
{}


bool Texture::create(const Vec2u& size, ColorFormat format, TextureFlags flags)
{
    destroy();
    m_size = size;
    m_format = format;
    m_flags = flags;

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
    ImageCreateInfo image_ci {size, vk_format(),
                             (has_mipmaps() ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0u) |
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT};
    m_image.create(image_ci.set_mip_levels(mip_levels()));
    m_image_view.create(device(), m_image.vk(), vk_format(), VK_IMAGE_ASPECT_COLOR_BIT, mip_levels());

    return true;
}


static constexpr size_t format_pixel_size(ColorFormat format) {
    switch (format) {
        case ColorFormat::LinearGrey:
            return 1;
        case ColorFormat::LinearBGRA:
        case ColorFormat::BGRA:
            return 4;
    }
    XCI_UNREACHABLE;
}


void Texture::write(const void* pixels)
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

    CommandBuffers cmd_buf(m_renderer);
    cmd_buf.create(m_renderer.vk_transient_command_pool(), 1);
    cmd_buf.begin();

    cmd_buf.transition_image_layout(m_image.vk(),
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            m_image_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0, mip_levels());

    if (m_pending_clear) {
        m_pending_clear = false;
        VkImageSubresourceRange range {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = mip_levels(),
                .baseArrayLayer = 0,
                .layerCount = 1,
        };
        VkClearColorValue clear_color {};
        vkCmdClearColorImage(cmd_buf.vk(),
                m_image.vk(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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
                m_size.x, m_image.vk(), region);
    }
    m_pending_regions.clear();

    if (has_mipmaps()) {
        generate_mipmaps(cmd_buf);
    } else {
        m_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        cmd_buf.transition_image_layout(m_image.vk(),
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_image_layout);
    }

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
        case ColorFormat::LinearGrey: return VK_FORMAT_R8_UNORM;
        case ColorFormat::LinearBGRA: return VK_FORMAT_B8G8R8A8_UNORM;
        case ColorFormat::BGRA: return VK_FORMAT_B8G8R8A8_SRGB;
    }
    XCI_UNREACHABLE;
}


VkDevice Texture::device() const
{
    return m_renderer.vk_device();
}


void Texture::generate_mipmaps(CommandBuffers& cmd_buf)
{
    Vec2i mip_size = Vec2i(m_size);
    const auto num_levels = mip_levels();
    for (uint32_t i = 1; i < num_levels; i++) {
        cmd_buf.transition_image_layout(m_image.vk(),
                                        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                        i - 1, 1);

        VkImageBlit blit {
            .srcSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = i - 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
            .srcOffsets = {{ 0, 0, 0 }, {mip_size.x, mip_size.y, 1}},
            .dstSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = i,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
            .dstOffsets = {{ 0, 0, 0 }, { mip_size.x > 1 ? mip_size.x / 2 : 1,
                                          mip_size.y > 1 ? mip_size.y / 2 : 1, 1 }},
        };
        vkCmdBlitImage(cmd_buf.vk(),
                       m_image.vk(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       m_image.vk(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit,
                       VK_FILTER_LINEAR);

        cmd_buf.transition_image_layout(m_image.vk(),
                                        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                        i - 1, 1);

        if (mip_size.x > 1)
            mip_size.x /= 2;
        if (mip_size.y > 1)
            mip_size.y /= 2;
    }

    cmd_buf.transition_image_layout(m_image.vk(),
                                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    num_levels - 1, 1);
}


void Texture::destroy()
{
    m_pending_regions.clear();
    if (m_staging_mapped != nullptr) {
        m_staging_memory.unmap();
        m_staging_mapped = nullptr;
    }
    vkDestroyBuffer(device(), m_staging_buffer, nullptr);
    m_image_view.destroy(device());
}


} // namespace xci::graphics
