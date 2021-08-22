// DescriptorPool.h created on 2021-08-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)


#ifndef XCI_GRAPHICS_VULKAN_DESCRIPTOR_POOL_H
#define XCI_GRAPHICS_VULKAN_DESCRIPTOR_POOL_H

#include <vulkan/vulkan.h>
#include <initializer_list>

namespace xci::graphics {

class Renderer;


class DescriptorPool {
public:
    explicit DescriptorPool(Renderer& renderer) : m_renderer(renderer) {}
    ~DescriptorPool() { destroy(); }

    void create(uint32_t max_sets, std::initializer_list<VkDescriptorPoolSize> pool_sizes);
    void destroy();

    void allocate(uint32_t count, const VkDescriptorSetLayout* layouts, VkDescriptorSet* descriptor_sets);
    void free(uint32_t count, const VkDescriptorSet* descriptor_sets);

private:
    Renderer& m_renderer;
    VkDescriptorPool m_descriptor_pool {};
};


} // namespace xci::graphics

#endif // include guard
