// DescriptorPool.cpp created on 2021-08-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "DescriptorPool.h"
#include <xci/graphics/Renderer.h>
#include "VulkanError.h"

namespace xci::graphics {


DescriptorPool::~DescriptorPool()
{
    if (m_renderer.vk_device() != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(m_renderer.vk_device(), m_descriptor_pool, nullptr);
}


void DescriptorPool::create(
        uint32_t max_sets, std::initializer_list<VkDescriptorPoolSize> pool_sizes)
{
    VkDescriptorPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = max_sets,
            .poolSizeCount = (uint32_t) pool_sizes.size(),
            .pPoolSizes = pool_sizes.begin(),
    };
    VK_TRY("vkCreateDescriptorPool",
            vkCreateDescriptorPool(m_renderer.vk_device(), &pool_info, nullptr,
                    &m_descriptor_pool));
}


void DescriptorPool::allocate(
        uint32_t count, const VkDescriptorSetLayout* layouts, VkDescriptorSet* descriptor_sets)
{
    VkDescriptorSetAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptor_pool,
            .descriptorSetCount = count,
            .pSetLayouts = layouts,
    };

    VK_TRY("vkAllocateDescriptorSets",
            vkAllocateDescriptorSets(m_renderer.vk_device(), &alloc_info,
                    descriptor_sets));
}


void DescriptorPool::free(uint32_t count, const VkDescriptorSet* descriptor_sets)
{
    VK_TRY("vkFreeDescriptorSets",
            vkFreeDescriptorSets(m_renderer.vk_device(), m_descriptor_pool,
                    count, descriptor_sets));
}


} // namespace xci::graphics
