// DeviceMemory.h created on 2019-12-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_VULKAN_DEVICE_MEMORY_H
#define XCI_GRAPHICS_VULKAN_DEVICE_MEMORY_H

#include <vulkan/vulkan.h>

namespace xci::graphics {

class Renderer;


/// Static memory allocator.
/// All memory blocks must be reserved first, then the whole chunk
/// is allocated and later freed. Dynamic allocations are not supported.
class DeviceMemory {
public:
    explicit DeviceMemory(Renderer& renderer) : m_renderer(renderer) {}
    ~DeviceMemory() { free(); }

    /// Reserve memory in pool
    /// \param requirements     allocation requirements as returned by
    ///                         vkGetBufferMemoryRequirements
    /// \return offset into device memory allocation
    [[nodiscard]]
    VkDeviceSize reserve(const VkMemoryRequirements& requirements);

    void allocate(VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void free();

    void bind_buffer(VkBuffer buffer, VkDeviceSize offset);
    void bind_image(VkImage image, VkDeviceSize offset);
    void copy_data(VkDeviceSize offset, VkDeviceSize size,
            const void* src_data);

    void* map(VkDeviceSize offset, VkDeviceSize size);
    void unmap();

private:
    uint32_t find_memory_type(VkMemoryPropertyFlags properties);

private:
    Renderer& m_renderer;
    VkDeviceMemory m_memory_pool { VK_NULL_HANDLE };
    VkDeviceSize m_alloc_size = 0;
    uint32_t m_type_bits = 0;
};


} // namespace xci::graphics

#endif // include guard
