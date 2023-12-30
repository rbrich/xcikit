// Sampler.h created on 2023-12-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_VULKAN_SAMPLER_H
#define XCI_GRAPHICS_VULKAN_SAMPLER_H

#include <vulkan/vulkan.h>
#include <functional>  // struct hash

namespace xci::graphics {


enum class SamplerAddressMode {
    Repeat = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    MirroredRepeat = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    ClampToEdge = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    ClampToBorder = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
};


class SamplerCreateInfo {
public:
    SamplerCreateInfo(SamplerAddressMode address_mode, float anisotropy);

    const VkSamplerCreateInfo& vk() const { return m_sampler_ci; }

    size_t hash() const;

    bool operator==(const SamplerCreateInfo& rhs) const;

private:
    VkSamplerCreateInfo m_sampler_ci;
};


/// Texture sampler
/// Do not create directly, use Renderer::get_sampler()
class Sampler {
public:
    bool create(VkDevice device, const SamplerCreateInfo& sampler_ci);
    void destroy(VkDevice device);

    // Vulkan handle
    VkSampler vk() const { return m_sampler; }

private:
    VkSampler m_sampler {};
};


} // namespace xci::graphics


namespace std {
template <> struct hash<xci::graphics::SamplerCreateInfo> {
    size_t operator()(const xci::graphics::SamplerCreateInfo &x) const { return x.hash(); }
};
} // namespace std


#endif  // XCI_GRAPHICS_VULKAN_SAMPLER_H
