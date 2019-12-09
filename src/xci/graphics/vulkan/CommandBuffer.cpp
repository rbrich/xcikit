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
        VkImageLayout old_layout, VkImageLayout new_layout)
{
    VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
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

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (
            old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
            new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    ) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    } else if (
            old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    ) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    } else {
        assert(!"transition_image_layout: unsupported arguments");
        return;
    }

    vkCmdPipelineBarrier(
            m_command_buffer,
            source_stage, destination_stage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
    );
}


void
CommandBuffer::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkBufferImageCopy region = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, 1},
    };
    vkCmdCopyBufferToImage(m_command_buffer, buffer, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}


} // namespace xci::graphics
