// DeviceMemory.cpp created on 2019-12-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "DeviceMemory.h"
#include <xci/graphics/Renderer.h>
#include <xci/core/memory.h>
#include "VulkanError.h"
#include <cstring>
#include <cassert>

namespace xci::graphics {

using core::align_to;


VkDeviceSize DeviceMemory::reserve(const VkMemoryRequirements& requirements)
{
    assert(m_memory_pool == VK_NULL_HANDLE);  // not yet allocated

    if (m_alloc_size == 0) {
        m_type_bits = requirements.memoryTypeBits;
        m_alloc_size += requirements.size;
        return 0;
    }

    m_type_bits &= requirements.memoryTypeBits;
    m_alloc_size = align_to(m_alloc_size, requirements.alignment);
    VkDeviceSize offset = m_alloc_size;
    m_alloc_size += requirements.size;
    return offset;
}


void DeviceMemory::allocate(VkMemoryPropertyFlags properties)
{
    if (m_alloc_size == 0)
        return;
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
    if (m_memory_pool) {
        vkFreeMemory(m_renderer.vk_device(), m_memory_pool, nullptr);
        m_memory_pool = VK_NULL_HANDLE;
        m_alloc_size = 0;
        m_type_bits = 0;
    }
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


void DeviceMemory::flush(std::span<MappedMemoryRange> ranges)
{
    const auto atom_size = m_renderer.non_coherent_atom_size();
    std::vector<VkMappedMemoryRange> vk_ranges;
    vk_ranges.reserve(ranges.size());
    for (const auto& range : ranges) {
        vk_ranges.push_back({
                .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                .memory = m_memory_pool,
                .offset = range.offset,
                .size = align_to(range.size, atom_size)
        });
    }
    VK_TRY("vkFlushMappedMemoryRanges",
           vkFlushMappedMemoryRanges(m_renderer.vk_device(), vk_ranges.size(), vk_ranges.data()));
}


void DeviceMemory::flush(VkDeviceSize offset, VkDeviceSize size)
{
    VkMappedMemoryRange range{
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .memory = m_memory_pool,
            .offset = offset,
            .size = align_to(size, m_renderer.non_coherent_atom_size()),
    };
    VK_TRY("vkFlushMappedMemoryRanges",
           vkFlushMappedMemoryRanges(m_renderer.vk_device(), 1, &range));
}


void DeviceMemory::invalidate(VkDeviceSize offset, VkDeviceSize size)
{
    VkMappedMemoryRange range{
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .memory = m_memory_pool,
            .offset = offset,
            .size = align_to(size, m_renderer.non_coherent_atom_size()),
    };
    VK_TRY("vkInvalidateMappedMemoryRanges",
           vkInvalidateMappedMemoryRanges(m_renderer.vk_device(), 1, &range));
}


static uint32_t lookup_memory_type(const VkPhysicalDeviceMemoryProperties& mem_props,
                              VkMemoryPropertyFlags properties, uint32_t type_bits)
{
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((type_bits & (1 << i))
            && (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    return ~0u;
}


uint32_t DeviceMemory::find_memory_type(VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(m_renderer.vk_physical_device(), &mem_props);

    auto found = lookup_memory_type(mem_props, properties, m_type_bits);
    if (found != ~0u)
        return found;

    // VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT is optional, try again without it
    if (properties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
        properties ^= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
        found = lookup_memory_type(mem_props, properties, m_type_bits);
        if (found != ~0u)
            return found;
    }

    VK_THROW("vkGetPhysicalDeviceMemoryProperties didn't return suitable memory type");
}


} // namespace xci::graphics
