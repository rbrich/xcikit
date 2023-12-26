// Image.h created on 2023-12-26 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_VULKAN_IMAGE_H
#define XCI_GRAPHICS_VULKAN_IMAGE_H

#include <xci/math/Vec2.h>
#include <xci/core/mixin.h>
#include <xci/graphics/vulkan/DeviceMemory.h>

#include <vulkan/vulkan.h>

namespace xci::graphics {


class ImageCreateInfo {
public:
    ImageCreateInfo(const Vec2u& size, VkFormat format, VkImageUsageFlags usage);

    const VkImageCreateInfo& vk() const { return m_image_ci; }

private:
    VkImageCreateInfo m_image_ci;
};


class Image: private core::NonCopyable {
public:
    explicit Image(Renderer& renderer) : m_renderer(renderer), m_image_memory(renderer) {}
    explicit Image(Renderer& renderer, const ImageCreateInfo& image_ci) : Image(renderer) { create(image_ci); }
    ~Image();

    void create(const ImageCreateInfo& image_ci);

    VkImage vk() const { return m_image; }

private:
    Renderer& m_renderer;
    VkImage m_image = VK_NULL_HANDLE;
    DeviceMemory m_image_memory;  // FIXME: pool the memory
};


class ImageView {
public:
    void create(VkDevice device, VkImage image, VkFormat format);
    void destroy(VkDevice device);

    const VkImageView& vk() const { return m_image_view; }

private:
    VkImageView m_image_view = VK_NULL_HANDLE;
};


} // namespace xci::graphics

#endif  // XCI_GRAPHICS_VULKAN_IMAGE_H
