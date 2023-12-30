// Sampler.cpp created on 2023-12-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Sampler.h"
#include "VulkanError.h"
#include <bit>

namespace xci::graphics {


SamplerCreateInfo::SamplerCreateInfo(SamplerAddressMode address_mode, float anisotropy)
    : m_sampler_ci {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VkSamplerAddressMode(address_mode),
            .addressModeV = VkSamplerAddressMode(address_mode),
            .anisotropyEnable = anisotropy == 0.0 ? VK_FALSE : VK_TRUE,
            .maxAnisotropy = anisotropy,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
    }
{}


size_t SamplerCreateInfo::hash() const
{
    size_t h = size_t(m_sampler_ci.maxAnisotropy);
    h = std::rotl(h, 2) ^ size_t(m_sampler_ci.addressModeU);
    return h;
}


bool SamplerCreateInfo::operator==(const SamplerCreateInfo& rhs) const
{
    return std::tie(m_sampler_ci.magFilter, m_sampler_ci.minFilter, m_sampler_ci.mipmapMode,
                    m_sampler_ci.addressModeU, m_sampler_ci.addressModeV,
                    m_sampler_ci.maxAnisotropy) ==
           std::tie(rhs.m_sampler_ci.magFilter, rhs.m_sampler_ci.minFilter, rhs.m_sampler_ci.mipmapMode,
                    rhs.m_sampler_ci.addressModeU, rhs.m_sampler_ci.addressModeV,
                    rhs.m_sampler_ci.maxAnisotropy);
}


bool Sampler::create(VkDevice device, const SamplerCreateInfo& sampler_ci)
{
    VK_TRY_RET("vkCreateSampler",
               vkCreateSampler(device, &sampler_ci.vk(), nullptr, &m_sampler));
    return true;
}


void Sampler::destroy(VkDevice device)
{
    vkDestroySampler(device, m_sampler, nullptr);
}


} // namespace xci::graphics
