// Texture.h created on 2018-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)


#ifndef XCI_GRAPHICS_TEXTURE_H
#define XCI_GRAPHICS_TEXTURE_H

#include <xci/math/Vec2.h>
#include <xci/math/Rect.h>
#include "vulkan/DeviceMemory.h"
#include "vulkan/Buffer.h"
#include "vulkan/Image.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <cstdint>
#include <vector>

namespace xci::graphics {

class Renderer;
class CommandBuffer;

using std::uint8_t;


enum class ColorFormat {
    BGRA,          // 32bit color in sRGB colorspace (standard color texture)
    LinearGrey,    // 256 shades of grey (linear intensity, e.g. font texture)
    LinearBGRA,    // 32bit color in linear colorspace (e.g. normal-mapping texture)
};

enum class TextureFlags : uint8_t {
    None            = 0x00,
    Mipmaps         = 0x01,  // generate mipmaps for the texture
};
inline TextureFlags operator|(TextureFlags a, TextureFlags b) { return TextureFlags(uint8_t(a) | uint8_t(b)); }
inline TextureFlags operator&(TextureFlags a, TextureFlags b) { return TextureFlags(uint8_t(a) & uint8_t(b)); }


uint32_t mip_levels_for_size(Vec2u size);


class Texture {
public:
    explicit Texture(Renderer& renderer);
    ~Texture() { destroy(); }

    // Create or resize the texture
    bool create(const Vec2u& size, ColorFormat format, TextureFlags flags = TextureFlags::None);

    // Write data to staging memory (don't forget to `update` the texture)
    void write(const void* pixels);
    void write(const uint8_t* pixels, const Rect_u& region);
    void clear();

    // Transfer pending data to texture memory, generate mipmaps
    void update();

    Vec2u size() const { return m_size; }
    VkDeviceSize byte_size() const;
    ColorFormat color_format() const { return m_format; }
    bool has_mipmaps() const { return (m_flags & TextureFlags::Mipmaps) == TextureFlags::Mipmaps; }
    uint32_t mip_levels() const { return has_mipmaps() ? mip_levels_for_size(m_size) : 1; }

    // Vulkan handles
    VkImageView vk_image_view() const { return m_image_view.vk(); }

private:
    VkFormat vk_format() const;
    VkDevice device() const;
    void generate_mipmaps(CommandBuffer& cmd_buf);
    void destroy();

    Renderer& m_renderer;
    ColorFormat m_format = ColorFormat::LinearGrey;
    Vec2u m_size;
    Buffer m_staging_buffer;
    Image m_image;
    ImageView m_image_view;
    VkImageLayout m_image_layout { VK_IMAGE_LAYOUT_UNDEFINED };
    DeviceMemory m_staging_memory;  // FIXME: pool the memory
    void* m_staging_mapped = nullptr;
    std::vector<Rect_u> m_pending_regions;
    TextureFlags m_flags = TextureFlags::None;
    bool m_pending_clear = false;
};


} // namespace xci::graphics

#endif // include guard
