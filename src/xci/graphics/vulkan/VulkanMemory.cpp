// VulkanMemory.cpp created on 2019-12-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "VulkanMemory.h"
#include "VulkanRenderer.h"
#include "VulkanError.h"
#include <cstring>
#include <cassert>

namespace xci::graphics {


VkDeviceSize VulkanMemory::reserve(const VkMemoryRequirements& requirements)
{
    assert(m_memory_pool == VK_NULL_HANDLE);  // not yet allocated

    if (m_alloc_size == 0) {
        m_type_bits = requirements.memoryTypeBits;
        m_alloc_size += requirements.size;
        return 0;
    }

    m_type_bits &= requirements.memoryTypeBits;
    pad_to_alignment(requirements.alignment);
    VkDeviceSize offset = m_alloc_size;
    m_alloc_size += requirements.size;
    return offset;
}


void VulkanMemory::allocate()
{
    VkMemoryAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = m_alloc_size,
            .memoryTypeIndex = find_memory_type(
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };
    VK_TRY("vkAllocateMemory (vertex/index buffer)",
            vkAllocateMemory(m_renderer.vk_device(), &alloc_info,
                    nullptr, &m_memory_pool));
}


void VulkanMemory::free()
{
    vkFreeMemory(m_renderer.vk_device(), m_memory_pool, nullptr);
    m_memory_pool = VK_NULL_HANDLE;
}


void VulkanMemory::bind_buffer(VkBuffer buffer, VkDeviceSize offset)
{
    assert(m_memory_pool != VK_NULL_HANDLE);  // must be allocated
    VK_TRY("vkBindBufferMemory",
            vkBindBufferMemory(m_renderer.vk_device(), buffer,
                    m_memory_pool, offset));
}


void VulkanMemory::copy_data(VkDeviceSize offset, VkDeviceSize size,
        const void* src_data)
{
    assert(m_memory_pool != VK_NULL_HANDLE);  // must be allocated
    void* mapped = nullptr;
    VK_TRY("vkMapMemory",
            vkMapMemory(m_renderer.vk_device(), m_memory_pool,
            offset, size, 0, &mapped));
    std::memcpy(mapped, src_data, (size_t) size);
    vkUnmapMemory(m_renderer.vk_device(), m_memory_pool);
}


uint32_t VulkanMemory::find_memory_type(VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(m_renderer.vk_physical_device(), &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((m_type_bits & (1 << i))
            && (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    VK_THROW("vkGetPhysicalDeviceMemoryProperties didn't return suitable memory type");
}


void VulkanMemory::pad_to_alignment(VkDeviceSize alignment)
{
    auto padding = alignment - (m_alloc_size % alignment);
    m_alloc_size += (padding == alignment) ? 0 : padding;
}


} // namespace xci::graphics
