// Renderer.h created on 2018-04-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_RENDERER_H
#define XCI_GRAPHICS_RENDERER_H

#include "Shader.h"
#include <xci/geometry/Vec2.h>
#include <xci/vfs/Vfs.h>
#include <xci/config.h>
#include <xci/graphics/vulkan/Swapchain.h>
#include <xci/graphics/vulkan/Pipeline.h>
#include <xci/graphics/vulkan/DescriptorPool.h>

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <optional>
#include <memory>
#include <array>
#include <cstdint>

struct SDL_Window;

namespace xci::graphics {

using xci::vfs::Vfs;


class Renderer: private core::NonCopyable {
public:
    explicit Renderer(Vfs& vfs);
    ~Renderer();

    Vfs& vfs() { return m_vfs; }

    /// Presentation mode. Limits framerate, avoids tearing.
    /// - Immediate      - do not wait for vertical blank period
    /// - Mailbox        - driver waits, program doesn't (new request replaces old one)
    /// - Fifo*          - full vsync, requests are queued (*default)
    /// - FifoRelaxed    - mostly vsync, late frame can be displayed immediately
    void set_present_mode(PresentMode mode) { m_swapchain.set_present_mode(mode); }
    PresentMode present_mode() const { return m_swapchain.present_mode(); }

    void set_device_id(uint32_t device_id) { m_device_id = device_id; }

    // -------------------------------------------------------------------------
    // Device limits

    // Max texture size
    uint32_t max_image_dimension_2d() const { return m_max_image_dimension_2d; }
    VkDeviceSize min_uniform_offset_alignment() const { return m_min_uniform_offset_alignment; }

    // -------------------------------------------------------------------------
    // Shaders

    /// Get one of the predefined shaders
    /// \param shader_id Use `Custom` to create new shader
    /// \return shared_ptr to the shader or nullptr on error
    Shader& get_shader(ShaderId shader_id);

    /// Load one of the predefined shaders
    /// This creates new Shader object. Use `get_shader` to get one
    /// from the cache instead.
    bool load_shader(ShaderId shader_id, Shader& shader);

    void clear_shader_cache();

    // -------------------------------------------------------------------------
    // Pipelines

    PipelineLayout& get_pipeline_layout(const PipelineLayoutCreateInfo& ci);
    Pipeline& get_pipeline(const PipelineCreateInfo& ci);

    void clear_pipeline_cache();

    // -------------------------------------------------------------------------
    // Descriptor pools

    /// Get an existing descriptor pool or create a new one. All pools are created with constant
    /// maxSets size, usually much bigger than requested by reserved_sets parameter.
    /// Multiple requests will get the same pool. The returned object is a RAII helper which will
    /// release reserved_sets when disposed of, so the reserved capacity will become available
    /// to satisfy future requests.
    /// The pool_sizes given here are per descriptor set. They are hashed and used to lookup
    /// a specific pool in cache.
    /// \param reserved_sets    Specify how many sets will you allocate from the pool, at maximum.
    /// \param pool_sizes       Sizes *per single descriptor set*.
    /// \return A facade to actual pool, which is owned by Renderer and shared as its capacity allows.
    SharedDescriptorPool get_descriptor_pool(uint32_t reserved_sets, DescriptorPoolSizes pool_sizes);

    void clear_descriptor_pool_cache() { m_descriptor_pool.clear(); }

    // -------------------------------------------------------------------------
    // Surface

    bool create_surface(SDL_Window* window);
    void destroy_surface();
    void reset_framebuffer(VkExtent2D new_size = {UINT32_MAX, UINT32_MAX}) { m_swapchain.reset_framebuffer(new_size); }

    // Vulkan handles
    VkInstance vk_instance() const { return m_instance; }
    VkSurfaceKHR vk_surface() const { return m_surface; }
    VkDevice vk_device() const { return m_device; }
    VkPhysicalDevice vk_physical_device() const { return m_physical_device; }
    VkSwapchainKHR vk_swapchain() const { return m_swapchain.vk(); }
    VkQueue vk_queue() const { return m_queue; }
    VkCommandPool vk_command_pool() const { return m_command_pool; }
    VkCommandPool vk_transient_command_pool() const { return m_transient_command_pool; }
    VkExtent2D vk_image_extent() const { return m_swapchain.vk_image_extent(); }
    VkRenderPass vk_render_pass() const { return m_render_pass; }
    VkFramebuffer vk_framebuffer(uint32_t index) const { return m_swapchain.vk_framebuffer(index); }

private:
    bool create_instance(SDL_Window* window);
    void create_device();
    void destroy_device();
    void create_renderpass();
    void destroy_renderpass();

    std::optional<uint32_t> query_queue_families(VkPhysicalDevice device);

    void load_device_limits(const VkPhysicalDeviceLimits& limits);

    Vfs& m_vfs;
    static constexpr auto c_num_shaders = (size_t) ShaderId::NumItems_;
    std::array<std::unique_ptr<Shader>, c_num_shaders> m_shader = {};

    std::unordered_map<PipelineLayoutCreateInfo, PipelineLayout> m_pipeline_layout;
    std::unordered_map<PipelineCreateInfo, Pipeline> m_pipeline;
    std::unordered_map<DescriptorPoolSizes, std::vector<DescriptorPool>> m_descriptor_pool;

    VkInstance m_instance {};
    VkSurfaceKHR m_surface {};
    VkPhysicalDevice m_physical_device {};
    VkDevice m_device {};
    VkQueue m_queue {};
    Swapchain m_swapchain {*this};
    VkRenderPass m_render_pass {};
    VkCommandPool m_command_pool {};
    VkCommandPool m_transient_command_pool {};

#ifdef XCI_DEBUG_VULKAN
    VkDebugUtilsMessengerEXT m_debug_messenger {};
#endif

    uint32_t m_device_id = ~0u;  // requested deviceID

    // Device limits
    uint32_t m_max_image_dimension_2d = 0;
    VkDeviceSize m_min_uniform_offset_alignment = 0;
};


} // namespace xci::graphics

#endif // include guard
