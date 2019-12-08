// VulkanTexture.h created on 2019-10-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_VULKAN_TEXTURE_H
#define XCI_GRAPHICS_VULKAN_TEXTURE_H

#include <xci/graphics/Texture.h>
#include "DeviceMemory.h"

#include <vulkan/vulkan.h>

namespace xci::graphics {

class VulkanRenderer;


class VulkanTexture : public Texture {
public:
    explicit VulkanTexture(VulkanRenderer& renderer);
    ~VulkanTexture() override { destroy(); }

    bool create(const Vec2u& size) override;

    void update(const uint8_t* pixels) override;
    void update(const uint8_t* pixels, const Rect_u& region) override;

    Vec2u size() const override { return m_size; }
    VkDeviceSize byte_size() const { return 4 * m_size.x * m_size.y; }

    // Vulkan handles
    VkSampler vk_sampler() const { return m_sampler; }
    VkImageView vk_image_view() const { return m_image_view; }

private:
    VkDevice device() const;
    void destroy();

private:
    VulkanRenderer& m_renderer;
    Vec2u m_size;
    VkBuffer m_staging_buffer {};
    VkImage m_image {};
    VkImageView m_image_view {};
    VkSampler m_sampler {};
    DeviceMemory m_staging_memory;  // FIXME: pool the memory
    DeviceMemory m_image_memory;  // FIXME: pool the memory
};


} // namespace xci::graphics

#endif // include guard
