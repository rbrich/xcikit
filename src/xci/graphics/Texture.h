// Texture.h created on 2018-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)


#ifndef XCI_GRAPHICS_TEXTURE_H
#define XCI_GRAPHICS_TEXTURE_H

#include <xci/math/Vec2.h>
#include <xci/math/Rect.h>
#include "vulkan/DeviceMemory.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <cstdint>
#include <vector>

namespace xci::graphics {

class Renderer;

using xci::core::Vec2u;
using xci::core::Rect_u;
using std::uint8_t;


enum class ColorFormat {
    Grey,  // 256 shades of grey
    BGRA,  // 32bit color
};


/// Gray-scale texture - 1 byte per pixel
class Texture {
public:
    explicit Texture(Renderer& renderer, ColorFormat format);
    ~Texture() { destroy(); }

    // Create or resize the texture
    bool create(const Vec2u& size);

    // Write data to staging memory (don't forget to `update` the texture)
    void write(const uint8_t* pixels);
    void write(const uint8_t* pixels, const Rect_u& region);
    void clear();

    // Transfer pending data to texture memory
    void update();

    Vec2u size() const { return m_size; }
    VkDeviceSize byte_size() const;
    ColorFormat color_format() const { return m_format; }

    // Vulkan handles
    VkSampler vk_sampler() const { return m_sampler; }
    VkImageView vk_image_view() const { return m_image_view; }

private:
    VkFormat vk_format() const;
    VkDevice device() const;
    void destroy();

private:
    Renderer& m_renderer;
    ColorFormat m_format;
    Vec2u m_size;
    VkBuffer m_staging_buffer {};
    VkImage m_image {};
    VkImageView m_image_view {};
    VkImageLayout m_image_layout { VK_IMAGE_LAYOUT_UNDEFINED };
    VkSampler m_sampler {};
    DeviceMemory m_staging_memory;  // FIXME: pool the memory
    DeviceMemory m_image_memory;  // FIXME: pool the memory
    void* m_staging_mapped = nullptr;
    std::vector<Rect_u> m_pending_regions;
    bool m_pending_clear = false;
};


} // namespace xci::graphics

#endif // include guard
