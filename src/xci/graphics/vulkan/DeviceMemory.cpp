// DeviceMemory.cpp created on 2019-12-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "DeviceMemory.h"
#include <xci/graphics/Renderer.h>
#include "VulkanError.h"
#include <cstring>
#include <cassert>

namespace xci::graphics {


VkDeviceSize DeviceMemory::reserve(const VkMemoryRequirements& requirements)
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


void DeviceMemory::allocate(VkMemoryPropertyFlags properties)
{
    VkMemoryAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = m_alloc_size,
            .memoryTypeIndex = find_memory_type(properties),
    };
    VK_TRY("vkAllocateMemory (vertex/index buffer)",
            vkAllocateMemory(m_renderer.vk_device(), &alloc_info,
                    nullptr, &m_memory_pool));
}


void DeviceMemory::free()
{
    vkFreeMemory(m_renderer.vk_device(), m_memory_pool, nullptr);
    m_memory_pool = VK_NULL_HANDLE;
}


void DeviceMemory::bind_buffer(VkBuffer buffer, VkDeviceSize offset)
{
    assert(m_memory_pool != VK_NULL_HANDLE);  // must be allocated
    VK_TRY("vkBindBufferMemory",
            vkBindBufferMemory(m_renderer.vk_device(), buffer,
                    m_memory_pool, offset));
}


void DeviceMemory::bind_image(VkImage image, VkDeviceSize offset)
{
    assert(m_memory_pool != VK_NULL_HANDLE);  // must be allocated
    VK_TRY("vkBindImageMemory",
            vkBindImageMemory(m_renderer.vk_device(), image,
                    m_memory_pool, offset));
}


void DeviceMemory::copy_data(VkDeviceSize offset, VkDeviceSize size,
        const void* src_data)
{
    void* mapped = map(offset, size);
    std::memcpy(mapped, src_data, (size_t) size);
    unmap();
}


void* DeviceMemory::map(VkDeviceSize offset, VkDeviceSize size)
{
    assert(m_memory_pool != VK_NULL_HANDLE);  // must be allocated
    void* mapped = nullptr;
    VK_TRY("vkMapMemory",
            vkMapMemory(m_renderer.vk_device(), m_memory_pool,
                    offset, size, 0, &mapped));
    return mapped;
}


void DeviceMemory::unmap()
{
    vkUnmapMemory(m_renderer.vk_device(), m_memory_pool);
}


uint32_t DeviceMemory::find_memory_type(VkMemoryPropertyFlags properties)
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


void DeviceMemory::pad_to_alignment(VkDeviceSize alignment)
{
    auto padding = alignment - (m_alloc_size % alignment);
    m_alloc_size += (padding == alignment) ? 0 : padding;
}


} // namespace xci::graphics
