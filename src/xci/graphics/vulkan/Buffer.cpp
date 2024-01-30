// Buffer.cpp created on 2024-01-28 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Buffer.h"
#include "VulkanError.h"

namespace xci::graphics {


auto Buffer::create(VkDevice device, DeviceMemory& memory,
                    VkDeviceSize size, VkBufferUsageFlags usage,
                    VkSharingMode sharing) -> VkDeviceSize
{
    VkBufferCreateInfo buffer_ci = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = sharing,
    };
    VK_TRY("vkCreateBuffer",
           vkCreateBuffer(device, &buffer_ci, nullptr, &m_vk_buffer));

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(device, m_vk_buffer, &mem_req);
    return memory.reserve(mem_req);
}


void Buffer::destroy(VkDevice device)
{
    vkDestroyBuffer(device, m_vk_buffer, nullptr);
}


} // namespace xci::graphics
