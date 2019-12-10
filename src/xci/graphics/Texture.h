// Texture.h created on 2018-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)


#ifndef XCI_GRAPHICS_TEXTURE_H
#define XCI_GRAPHICS_TEXTURE_H

#include <xci/core/geometry.h>
#include "vulkan/DeviceMemory.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <cstdint>

namespace xci::graphics {

class Renderer;

using xci::core::Vec2u;
using xci::core::Rect_u;
using std::uint8_t;


/// Gray-scale texture - 1 byte per pixel
class Texture {
public:
    explicit Texture(Renderer& renderer);
    ~Texture() { destroy(); }

    // Create or resize the texture
    bool create(const Vec2u& size);
    void update(const uint8_t* pixels);
    void update(const uint8_t* pixels, const Rect_u& region);

    Vec2u size() const { return m_size; }
    VkDeviceSize byte_size() const { return m_size.x * m_size.y; }

    // Vulkan handles
    VkSampler vk_sampler() const { return m_sampler; }
    VkImageView vk_image_view() const { return m_image_view; }

private:
    VkDevice device() const;
    void destroy();

private:
    Renderer& m_renderer;
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
