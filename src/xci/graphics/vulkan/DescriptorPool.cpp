// DescriptorPool.cpp created on 2021-08-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "DescriptorPool.h"
#include "VulkanError.h"
#include <xci/graphics/Renderer.h>
#include <bit>
#include <cassert>


static bool operator==(const VkDescriptorPoolSize& lhs, const VkDescriptorPoolSize& rhs) {
    return lhs.type == rhs.type && lhs.descriptorCount == rhs.descriptorCount;
}


namespace xci::graphics {


void DescriptorPoolSizes::add(VkDescriptorType type, uint32_t count)
{
    assert(m_count < m_pool_sizes.size());
    m_pool_sizes[m_count++] = VkDescriptorPoolSize{.type = type, .descriptorCount = count};
}


size_t DescriptorPoolSizes::hash() const
{
    size_t h = 0;
    for (const auto& item : *this) {
        h = std::rotl(h, 1) ^ item.type ^ (item.descriptorCount << 4);
    }
    return h;
}


bool DescriptorPoolSizes::operator==(const DescriptorPoolSizes& rhs) const
{
    return m_count == rhs.m_count && std::equal(begin(), end(), rhs.begin());
}


// -----------------------------------------------------------------------------

void DescriptorPool::create(uint32_t max_sets, DescriptorPoolSizes pool_sizes)
{
    for (auto& item : pool_sizes)
        item.descriptorCount *= max_sets;

    VkDescriptorPoolCreateInfo ci = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = max_sets,
            .poolSizeCount = pool_sizes.size(),
            .pPoolSizes = pool_sizes.data(),
    };
    VK_TRY("vkCreateDescriptorPool",
            vkCreateDescriptorPool(m_renderer.vk_device(), &ci, nullptr,
                    &m_descriptor_pool));
    m_capacity = max_sets;
}


void DescriptorPool::destroy()
{
    if (m_descriptor_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_renderer.vk_device(), m_descriptor_pool, nullptr);
        m_descriptor_pool = VK_NULL_HANDLE;
    }
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


bool DescriptorPool::book_capacity(uint32_t count)
{
    if (m_capacity < count)
        return false;
    m_capacity -= count;
    return true;
}


SharedDescriptorPool::~SharedDescriptorPool()
{
    if (m_descriptor_pool && m_booked_sets > 0)
        m_descriptor_pool->unbook_capacity(m_booked_sets);
}


SharedDescriptorPool::SharedDescriptorPool(SharedDescriptorPool&& rhs)
    : m_descriptor_pool(rhs.m_descriptor_pool), m_booked_sets(0)
{
    std::swap(m_booked_sets, rhs.m_booked_sets);
}


SharedDescriptorPool& SharedDescriptorPool::operator=(SharedDescriptorPool&& rhs)
{
    std::swap(m_descriptor_pool, rhs.m_descriptor_pool);
    std::swap(m_booked_sets, rhs.m_booked_sets);
    return *this;
}


} // namespace xci::graphics
