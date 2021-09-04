// DescriptorPool.h created on 2021-08-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)


#ifndef XCI_GRAPHICS_VULKAN_DESCRIPTOR_POOL_H
#define XCI_GRAPHICS_VULKAN_DESCRIPTOR_POOL_H

#include <xci/core/mixin.h>
#include <vulkan/vulkan.h>
#include <array>

namespace xci::graphics {

class Renderer;


class DescriptorPoolSizes {
public:
    void add(VkDescriptorType type, uint32_t count);

    using const_iterator = const VkDescriptorPoolSize *;
    const_iterator begin() const { return m_pool_sizes.begin(); }
    const_iterator end() const { return m_pool_sizes.begin() + m_count; }

    using iterator = VkDescriptorPoolSize *;
    iterator begin() { return m_pool_sizes.begin(); }
    iterator end() { return m_pool_sizes.begin() + m_count; }

    uint32_t size() const { return m_count; }

    size_t hash() const;

    bool operator==(const DescriptorPoolSizes& rhs) const;

private:
    std::array<VkDescriptorPoolSize, 10> m_pool_sizes;
    uint32_t m_count = 0;
};


class DescriptorPool {
public:
    explicit DescriptorPool(Renderer& renderer) : m_renderer(renderer) {}
    ~DescriptorPool() { destroy(); }

    void create(uint32_t max_sets, DescriptorPoolSizes pool_sizes);
    void destroy();

    void allocate(uint32_t count, const VkDescriptorSetLayout* layouts, VkDescriptorSet* descriptor_sets);
    void free(uint32_t count, const VkDescriptorSet* descriptor_sets);

    /// Reserve part of capacity of the pool
    /// \return true if there was available capacity and it was reserved, false if not reserved
    bool book_capacity(uint32_t count);

    /// Return the reserved capacity back to the pool
    void unbook_capacity(uint32_t count) { m_capacity += count; }

private:
    Renderer& m_renderer;
    VkDescriptorPool m_descriptor_pool {};
    uint32_t m_capacity = 0;
};


class SharedDescriptorPool: private core::NonCopyable {
public:
    SharedDescriptorPool() {}
    explicit SharedDescriptorPool(DescriptorPool& pool, uint32_t booked_sets)
        : m_descriptor_pool(&pool), m_booked_sets(booked_sets) {}
    ~SharedDescriptorPool();

    SharedDescriptorPool(SharedDescriptorPool&& rhs);
    SharedDescriptorPool& operator=(SharedDescriptorPool&& rhs);

    DescriptorPool& get() const { return *m_descriptor_pool; }
    operator bool() const { return  m_descriptor_pool != nullptr; }

private:
    DescriptorPool* m_descriptor_pool = nullptr;
    uint32_t m_booked_sets;
};


} // namespace xci::graphics


namespace std {
    template <> struct hash<xci::graphics::DescriptorPoolSizes> {
        size_t operator()(const xci::graphics::DescriptorPoolSizes &x) const {
            return x.hash();
        }
    };
}


#endif // include guard
