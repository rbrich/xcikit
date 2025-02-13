// Renderer.h created on 2018-04-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_RENDERER_H
#define XCI_GRAPHICS_RENDERER_H

#include "Shader.h"
#include <xci/math/Vec2.h>
#include <xci/vfs/Vfs.h>
#include <xci/config.h>
#include <xci/graphics/vulkan/Swapchain.h>
#include <xci/graphics/vulkan/Pipeline.h>
#include <xci/graphics/vulkan/DescriptorPool.h>
#include <xci/graphics/vulkan/Sampler.h>

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <optional>
#include <map>
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

    /// Enable depth buffering.
    /// This is prerequisite for fragment depth test, which is enabled separately in Pipeline
    /// (possibly via Primitives).
    /// Default: disabled
    void set_depth_buffering(bool enable) { m_swapchain.set_depth_buffering(enable); }
    bool depth_buffering() const { return m_swapchain.depth_buffering(); }

    /// Multisampling (MSAA)
    void set_sample_count(uint32_t count) { m_swapchain.set_sample_count(count); }
    uint32_t sample_count() const { return m_swapchain.sample_count(); }
    uint32_t max_sample_count() const { return uint32_t(m_max_sample_count); }

    void set_device_id(uint32_t device_id) { m_device_id = device_id; }

    // -------------------------------------------------------------------------
    // Device limits

    // Max texture size
    uint32_t max_image_dimension_2d() const { return m_max_image_dimension_2d; }
    VkDeviceSize min_uniform_offset_alignment() const { return m_min_uniform_offset_alignment; }
    VkDeviceSize non_coherent_atom_size() const { return m_non_coherent_atom_size; }

    // -------------------------------------------------------------------------
    // Shaders

    /// Load or use cached shader modules to create a Shader program.
    /// \param vert_name    Name of shader in VFS ("shaders/<name>.vert")
    /// \param frag_name    Name of shader in VFS ("shaders/<name>.frag")
    /// \throws VulkanError on error
    Shader get_shader(std::string_view vert_name, std::string_view frag_name);

    /// Load shader module (vertex or fragment shader), or return a cached one.
    /// \returns Optional reference to the module. Null on failure.
    ShaderModule* load_shader_module(const std::string& vfs_path);

    void clear_shader_cache() { m_shader_module.clear(); }

    // -------------------------------------------------------------------------
    // Samplers

    float max_sampler_anisotropy() const { return m_max_sampler_anisotropy; }

    /// Get existing sampler or create a new one.
    /// \param address_mode     Addressing mode for both U, V coords
    /// \param anisotropy       Max anisotropy level. Use 0.f to disable.
    ///                         Capped at max_sampler_anisotropy(), which is usually 16.
    /// \param max_lod          Max mipmap level. Can be bigger than mip levels available in the texture.
    ///                         Set to 0 to disable mipmaps.
    Sampler& get_sampler(SamplerAddressMode address_mode = SamplerAddressMode::ClampToEdge,
                         float anisotropy = 0.f, unsigned max_lod = 16);

    void clear_sampler_cache();

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
    Swapchain& swapchain() { return m_swapchain; }

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
    VkRenderPass vk_render_pass() const { return m_swapchain.attachments().render_pass(); }
    VkFramebuffer vk_framebuffer(uint32_t index) const { return m_swapchain.vk_framebuffer(index); }

private:
    bool create_instance();
    void create_device();
    void destroy_device();

    std::optional<uint32_t> query_queue_families(VkPhysicalDevice device);

    void load_device_properties(const VkPhysicalDeviceProperties& props);

    Vfs& m_vfs;
    std::map<std::string, ShaderModule> m_shader_module = {};

    // Vulkan object deduplication caches
    std::unordered_map<PipelineLayoutCreateInfo, PipelineLayout> m_pipeline_layout;
    std::unordered_map<PipelineCreateInfo, Pipeline> m_pipeline;
    std::unordered_map<DescriptorPoolSizes, std::vector<DescriptorPool>> m_descriptor_pool;
    std::unordered_map<SamplerCreateInfo, Sampler> m_sampler;

    VkInstance m_instance {};
    VkSurfaceKHR m_surface {};
    VkPhysicalDevice m_physical_device {};
    VkDevice m_device {};
    VkQueue m_queue {};
    Swapchain m_swapchain {*this};
    VkCommandPool m_command_pool {};
    VkCommandPool m_transient_command_pool {};

#ifdef XCI_DEBUG_VULKAN
    VkDebugUtilsMessengerEXT m_debug_messenger {};
#endif

    uint32_t m_device_id = ~0u;  // requested deviceID

    // Device limits
    uint32_t m_max_image_dimension_2d = 0;
    VkDeviceSize m_min_uniform_offset_alignment = 0;
    VkDeviceSize m_non_coherent_atom_size = 0;
    float m_max_sampler_anisotropy = 0.0f;
    VkSampleCountFlagBits m_max_sample_count = VK_SAMPLE_COUNT_1_BIT;  // for MSAA
};


} // namespace xci::graphics

#endif // include guard
