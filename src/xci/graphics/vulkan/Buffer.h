// Buffer.h created on 2024-01-28 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_VULKAN_BUFFER_H
#define XCI_GRAPHICS_VULKAN_BUFFER_H

#include "DeviceMemory.h"
#include <vulkan/vulkan.h>

namespace xci::graphics {


class Buffer {
public:
    /// Create buffer on `device`, reserving space in `memory`.
    /// \returns    memory offset
    auto create(VkDevice device, DeviceMemory& memory,
                VkDeviceSize size, VkBufferUsageFlags usage,
                VkSharingMode sharing = VK_SHARING_MODE_EXCLUSIVE) -> VkDeviceSize;

    void destroy(VkDevice device);

    VkBuffer vk() const { return m_vk_buffer; }
    const VkBuffer* vk_p() const { return &m_vk_buffer; }

private:
    VkBuffer m_vk_buffer = nullptr;
};


} // namespace xci::graphics

#endif  // XCI_GRAPHICS_VULKAN_BUFFER_H
