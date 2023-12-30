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
#include "vulkan/Image.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <cstdint>
#include <vector>

namespace xci::graphics {

class Renderer;

using std::uint8_t;


enum class ColorFormat {
    BGRA,          // 32bit color in sRGB colorspace (standard color texture)
    LinearGrey,    // 256 shades of grey (linear intensity, e.g. font texture)
    LinearBGRA,    // 32bit color in linear colorspace (e.g. normal-mapping texture)
};


class Texture {
public:
    explicit Texture(Renderer& renderer);
    ~Texture() { destroy(); }

    // Create or resize the texture
    bool create(const Vec2u& size, ColorFormat format);

    // Write data to staging memory (don't forget to `update` the texture)
    void write(const void* pixels);
    void write(const uint8_t* pixels, const Rect_u& region);
    void clear();

    // Transfer pending data to texture memory
    void update();

    Vec2u size() const { return m_size; }
    VkDeviceSize byte_size() const;
    ColorFormat color_format() const { return m_format; }

    // Vulkan handles
    VkImageView vk_image_view() const { return m_image_view.vk(); }

private:
    VkFormat vk_format() const;
    VkDevice device() const;
    void destroy();

private:
    Renderer& m_renderer;
    ColorFormat m_format = ColorFormat::LinearGrey;
    Vec2u m_size;
    VkBuffer m_staging_buffer {};
    Image m_image;
    ImageView m_image_view;
    VkImageLayout m_image_layout { VK_IMAGE_LAYOUT_UNDEFINED };
    DeviceMemory m_staging_memory;  // FIXME: pool the memory
    void* m_staging_mapped = nullptr;
    std::vector<Rect_u> m_pending_regions;
    bool m_pending_clear = false;
};


} // namespace xci::graphics

#endif // include guard
