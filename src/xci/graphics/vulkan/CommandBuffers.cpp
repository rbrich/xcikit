// CommandBuffers.cpp created on 2019-12-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <cassert>
#include "CommandBuffers.h"
#include <xci/graphics/Renderer.h>
#include "VulkanError.h"

namespace xci::graphics {


CommandBuffers::~CommandBuffers()
{
    vkFreeCommandBuffers(m_renderer.vk_device(), m_command_pool,
            m_count, m_command_buffers.data());
}


void CommandBuffers::create(VkCommandPool command_pool, uint32_t count)
{
    assert(count <= max_count);
    assert(m_command_pool == VK_NULL_HANDLE);  // not yet created
    m_command_pool = command_pool;
    m_count = count;

    VkCommandBufferAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = m_count,
    };
    VK_TRY("vkAllocateCommandBuffers",
            vkAllocateCommandBuffers(m_renderer.vk_device(), &alloc_info,
                    m_command_buffers.data()));
}


void CommandBuffers::reset()
{
    for (unsigned i = 0; i != m_count; ++i) {
        if (m_command_buffers[i] != nullptr) {
            VK_TRY("vkResetCommandBuffer",
                    vkResetCommandBuffer(m_command_buffers[i], 0));
        }
    }
}


void CommandBuffers::begin(unsigned idx)
{
    VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_TRY("vkBeginCommandBuffer",
            vkBeginCommandBuffer(m_command_buffers[idx], &begin_info));
}


void CommandBuffers::end(unsigned idx)
{
    VK_TRY("vkEndCommandBuffer", vkEndCommandBuffer(m_command_buffers[idx]));
}


void CommandBuffers::submit(unsigned idx)
{
    VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &m_command_buffers[idx],
    };
    VK_TRY("vkQueueSubmit",
            vkQueueSubmit(m_renderer.vk_queue(), 1, &submit_info,
                    VK_NULL_HANDLE));
    VK_TRY("vkQueueWaitIdle", vkQueueWaitIdle(m_renderer.vk_queue()));
}


void CommandBuffers::transition_image_layout(VkImage image,
        VkAccessFlags src_access, VkAccessFlags dst_access,
        VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage,
        VkImageLayout old_layout, VkImageLayout new_layout)
{
    VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = src_access,
            .dstAccessMask = dst_access,
            .oldLayout = old_layout,
            .newLayout = new_layout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
    };
    vkCmdPipelineBarrier(
            m_command_buffers[0],
            src_stage, dst_stage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
    );
}


void CommandBuffers::transition_buffer(
    VkBuffer buffer, VkDeviceSize size,
    VkAccessFlags src_access, VkAccessFlags dst_access,
    VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
{
    VkBufferMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = src_access,
        .dstAccessMask = dst_access,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = buffer,
        .offset = 0,
        .size = size,
    };
    vkCmdPipelineBarrier(
        m_command_buffers[0],
        src_stage, dst_stage,
        0,
        0, nullptr,
        1, &barrier,
        0, nullptr
    );
}


void
CommandBuffers::copy_buffer_to_image(
        VkBuffer buffer, VkDeviceSize buffer_offset, uint32_t buffer_row_len,
        VkImage image, const Rect_u& region)
{
    VkBufferImageCopy copy_region = {
            .bufferOffset = buffer_offset,
            .bufferRowLength = buffer_row_len,
            .bufferImageHeight = 0,
            .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
            .imageOffset = {int32_t(region.x), int32_t(region.y), 0},
            .imageExtent = {region.w, region.h, 1},
    };
    vkCmdCopyBufferToImage(m_command_buffers[0], buffer, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copy_region);
}


} // namespace xci::graphics
