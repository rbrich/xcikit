// CommandBuffer.cpp created on 2019-12-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <cassert>
#include "CommandBuffer.h"
#include <xci/graphics/Renderer.h>
#include "VulkanError.h"

namespace xci::graphics {


CommandBuffer::CommandBuffer(Renderer& renderer)
    : m_renderer(renderer)
{
    VkCommandBufferAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_renderer.vk_transient_command_pool(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
    };
    VK_TRY("vkAllocateCommandBuffers",
            vkAllocateCommandBuffers(m_renderer.vk_device(), &alloc_info,
                    &m_command_buffer));
}


CommandBuffer::~CommandBuffer()
{
    vkFreeCommandBuffers(m_renderer.vk_device(), m_renderer.vk_transient_command_pool(),
            1, &m_command_buffer);
}


void CommandBuffer::begin()
{
    VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_TRY("vkBeginCommandBuffer",
            vkBeginCommandBuffer(m_command_buffer, &begin_info));
}


void CommandBuffer::end()
{
    VK_TRY("vkEndCommandBuffer", vkEndCommandBuffer(m_command_buffer));
}


void CommandBuffer::submit()
{
    VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &m_command_buffer,
    };
    VK_TRY("vkQueueSubmit",
            vkQueueSubmit(m_renderer.vk_queue(), 1, &submit_info,
                    VK_NULL_HANDLE));
    VK_TRY("vkQueueWaitIdle", vkQueueWaitIdle(m_renderer.vk_queue()));
}


void CommandBuffer::transition_image_layout(VkImage image,
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
            m_command_buffer,
            src_stage, dst_stage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
    );
}


void CommandBuffer::transition_buffer(
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
        m_command_buffer,
        src_stage, dst_stage,
        0,
        0, nullptr,
        1, &barrier,
        0, nullptr
    );
}


void
CommandBuffer::copy_buffer_to_image(VkBuffer buffer, VkImage image, const Rect_u& region)
{
    VkBufferImageCopy copy_region = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
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
    vkCmdCopyBufferToImage(m_command_buffer, buffer, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copy_region);
}


} // namespace xci::graphics
