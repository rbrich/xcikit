// CommandBuffers.cpp created on 2019-12-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <cassert>
#include "CommandBuffers.h"
#include <xci/graphics/Renderer.h>
#include "VulkanError.h"

namespace xci::graphics {


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
    std::array<VkCommandBuffer, max_count> pointers;
    VK_TRY("vkAllocateCommandBuffers",
            vkAllocateCommandBuffers(m_renderer.vk_device(), &alloc_info,
                                    pointers.data()));
    for (unsigned i = 0; i != m_count; ++i) {
        m_command_buffers[i].m_vk_command_buffer = pointers[i];
    }
}


void CommandBuffers::destroy()
{
    std::array<VkCommandBuffer, max_count> pointers;
    for (unsigned i = 0; i != m_count; ++i) {
        pointers[i] = m_command_buffers[i].vk();
    }
    if (m_command_pool != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(m_renderer.vk_device(), m_command_pool,
                             m_count, pointers.data());
        m_command_pool = VK_NULL_HANDLE;
        for (unsigned i = 0; i != m_count; ++i)
            m_command_buffers[i].release_resources();
    }
}


void CommandBuffer::reset()
{
    if (m_vk_command_buffer) {
        VK_TRY("vkResetCommandBuffer",
                vkResetCommandBuffer(m_vk_command_buffer, 0));
    }
}


void CommandBuffer::begin()
{
    VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_TRY("vkBeginCommandBuffer",
            vkBeginCommandBuffer(m_vk_command_buffer, &begin_info));
}


void CommandBuffer::end()
{
    VK_TRY("vkEndCommandBuffer", vkEndCommandBuffer(m_vk_command_buffer));
}


void CommandBuffer::submit(VkQueue queue, VkFence fence)
{
    VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &m_vk_command_buffer,
    };
    VK_TRY("vkQueueSubmit",
            vkQueueSubmit(queue, 1, &submit_info, fence));
    VK_TRY("vkQueueWaitIdle", vkQueueWaitIdle(queue));
}


void CommandBuffer::submit(VkQueue queue, VkSemaphore wait, VkPipelineStageFlags wait_stage,
                           VkSemaphore signal, VkFence fence)
{
    const VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &wait,
            .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = &m_vk_command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &signal,
    };
    VK_TRY("vkQueueSubmit",
           vkQueueSubmit(queue, 1, &submit_info, fence));
}


void CommandBuffers::reset()
{
    for (unsigned i = 0; i != m_count; ++i) {
        m_command_buffers[i].reset();
    }
}


void CommandBuffers::submit(unsigned int idx)
{
    m_command_buffers[idx].submit(m_renderer.vk_queue());
}


void CommandBuffer::set_viewport(Vec2f size, bool flipped_y)
{
    VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = size.x,
            .height = size.y,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    if (flipped_y) {
        viewport.y = viewport.height;
        viewport.height = -viewport.height;
    }
    vkCmdSetViewport(m_vk_command_buffer, 0, 1, &viewport);
}


void CommandBuffer::transition_image_layout(VkImage image,
        VkAccessFlags src_access, VkAccessFlags dst_access,
        VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage,
        VkImageLayout old_layout, VkImageLayout new_layout,
        uint32_t mip_base, uint32_t mip_count)
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
                    .baseMipLevel = mip_base,
                    .levelCount = mip_count,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
    };
    vkCmdPipelineBarrier(
            m_vk_command_buffer,
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
        m_vk_command_buffer,
        src_stage, dst_stage,
        0,
        0, nullptr,
        1, &barrier,
        0, nullptr
    );
}


void CommandBuffer::copy_buffer_to_image(
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
    vkCmdCopyBufferToImage(m_vk_command_buffer, buffer, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copy_region);
}


void CommandBuffer::copy_image_to_buffer(
        VkImage image, const Rect_u& region,
        VkBuffer buffer, VkDeviceSize buffer_offset, uint32_t buffer_row_len)
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
    vkCmdCopyImageToBuffer(m_vk_command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           buffer, 1, &copy_region);
}


void CommandBuffer::release_resources()
{
    for (auto& deleter : m_resources)
        deleter();
    m_resources.clear();
}


void CommandBuffers::remove_callbacks(void* owner)
{
    std::erase_if(m_callbacks, [owner](const CallbackInfo& info) {
        return info.owner == owner;
    });
}


void CommandBuffers::trigger_callbacks(Event event, unsigned i, uint32_t image_index)
{
    CommandBuffer& cmd_buf = m_command_buffers[i];
    for (const CallbackInfo& info : m_callbacks) {
        if (info.event == event) {
            info.cb(cmd_buf, image_index);
        }
    }
}


} // namespace xci::graphics
